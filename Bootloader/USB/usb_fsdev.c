#include "usb_fsdev.h"
#include "usb_msc.h"
#include "usb_descriptors.h"
#include "stm32f303_usb.h"
#include "stm32f3xx.h"
#include <string.h>
#define EP0_RX_PMA 64U
#define EP0_TX_PMA 128U
#define EP1_TX_PMA 192U
#define EP2_RX_PMA 256U
#define PMA_WORD(a) ((volatile uint16_t *)(USB_PMA_BASE+((uint32_t)(a)*USB_PMA_ACCESS)))
static USB_SetupPacket_t setup;
static uint8_t ctrl_buf[64];
static const uint8_t *ctrl_ptr;
static uint16_t ctrl_len,ctrl_sent;
static uint8_t pending_addr=0xFFU;
static uint8_t configured;
static uint8_t ctrl_state;
#define CTRL_IDLE 0U
#define CTRL_DATA 1U
#define CTRL_STATUS_IN 2U
#define CTRL_STATUS_OUT 3U
static volatile uint16_t *epreg(uint8_t ep){return (volatile uint16_t *)((uintptr_t)&USB->EP0R+ep*2U);}
static volatile uint16_t *btw(uint8_t ep,uint8_t n){return (volatile uint16_t *)(USB_PMA_BASE+((ep*8U+n)*USB_PMA_ACCESS));}
static void pma_write(uint16_t a,const uint8_t *p,uint16_t n){
 volatile uint16_t *d=PMA_WORD(a);uint16_t i=0;
 while(i+1<n){*d=(uint16_t)p[i]|((uint16_t)p[i+1]<<8);d+=2;i+=2;}
 if(i<n)*d=p[i];
}
static void pma_read(uint16_t a,uint8_t *p,uint16_t n){
 volatile uint16_t *s=PMA_WORD(a);uint16_t i=0,v;
 while(i+1<n){v=*s;p[i]=(uint8_t)v;p[i+1]=(uint8_t)(v>>8);s+=2;i+=2;}
 if(i<n)*p=(uint8_t)*s;
}
static uint16_t rx_count_code(uint16_t n){
 uint16_t b;
 if(n==0)return 0x8000U;
 if(n<=62){b=(uint16_t)((n+1U)/2U);return (uint16_t)(b<<10);}
 b=(uint16_t)((n+31U)/32U);return (uint16_t)(0x8000U|(b<<10));
}
static uint16_t rx_count(uint8_t ep){return (uint16_t)(*btw(ep,3)&0x3ffU);}
static void set_tx_count(uint8_t ep,uint16_t n){*btw(ep,1)=n;}
static void set_rx_count(uint8_t ep,uint16_t n){*btw(ep,3)=rx_count_code(n);}
static uint16_t er(uint8_t ep){return *epreg(ep);}
static void ew(uint8_t ep,uint16_t v){*epreg(ep)=v;}
static void set_tx(uint8_t ep,uint16_t s){
 uint16_t r=(uint16_t)(er(ep)&USB_EPTX_DTOGMASK);
 if(s&USB_EPTX_DTOG1)r^=USB_EPTX_DTOG1;
 if(s&USB_EPTX_DTOG2)r^=USB_EPTX_DTOG2;
 ew(ep,(uint16_t)(r|USB_EP_CTR_RX|USB_EP_CTR_TX));
}
static void set_rx(uint8_t ep,uint16_t s){
 uint16_t r=(uint16_t)(er(ep)&USB_EPRX_DTOGMASK);
 if(s&USB_EPRX_DTOG1)r^=USB_EPRX_DTOG1;
 if(s&USB_EPRX_DTOG2)r^=USB_EPRX_DTOG2;
 ew(ep,(uint16_t)(r|USB_EP_CTR_RX|USB_EP_CTR_TX));
}
static void clear_rx_ctr(uint8_t ep){
 uint16_t r=(uint16_t)(er(ep)&(USB_EPREG_MASK&~USB_EP_CTR_RX));
 ew(ep,(uint16_t)(r|USB_EP_CTR_TX));
}
static void clear_tx_ctr(uint8_t ep){
 uint16_t r=(uint16_t)(er(ep)&(USB_EPREG_MASK&~USB_EP_CTR_TX));
 ew(ep,(uint16_t)(r|USB_EP_CTR_RX));
}
static void toggle_reset(uint8_t ep,bool in){
 uint16_t m=in?USB_EP_DTOG_TX:USB_EP_DTOG_RX,r=er(ep);
 if(r&m){r=(uint16_t)((r&USB_EPREG_MASK)|m|USB_EP_CTR_RX|USB_EP_CTR_TX);ew(ep,r);}
}
static void configure_ep(uint8_t ep,uint16_t type,uint16_t tx,uint16_t rx){
 ew(ep,(uint16_t)(type|ep|USB_EP_CTR_RX|USB_EP_CTR_TX));
 toggle_reset(ep,false);toggle_reset(ep,true);
 if(tx)set_tx_count(ep,0);
 if(rx)set_rx_count(ep,rx);
 set_tx(ep,USB_EP_TX_NAK);
 set_rx(ep,rx?USB_EP_RX_VALID:USB_EP_RX_NAK);
}
static void ctrl_setup_ready(void){set_rx_count(0,64);set_tx(0,USB_EP_TX_NAK);set_rx(0,USB_EP_RX_VALID);}
static void ctrl_stall(void){ctrl_state=CTRL_IDLE;set_tx(0,USB_EP_TX_STALL);set_rx(0,USB_EP_RX_STALL);}
static void ctrl_status(void){ctrl_state=CTRL_STATUS_IN;set_tx_count(0,0);set_rx(0,USB_EP_RX_NAK);set_tx(0,USB_EP_TX_VALID);}
static void ctrl_send_next(void){
 uint16_t n;
 if(ctrl_sent<ctrl_len){
  n=(uint16_t)(ctrl_len-ctrl_sent);if(n>64)n=64;
  pma_write(EP0_TX_PMA,&ctrl_ptr[ctrl_sent],n);set_tx_count(0,n);ctrl_sent=(uint16_t)(ctrl_sent+n);
  set_tx(0,USB_EP_TX_VALID);set_rx(0,USB_EP_RX_NAK);return;
 }
 ctrl_state=CTRL_STATUS_OUT;set_tx(0,USB_EP_TX_NAK);set_rx(0,USB_EP_RX_VALID);
}
static void ctrl_data_in(const uint8_t *p,uint16_t n){
 ctrl_ptr=p;ctrl_len=n;ctrl_sent=0;ctrl_state=CTRL_DATA;set_rx(0,USB_EP_RX_NAK);ctrl_send_next();
}
static bool std_request(void){
 uint8_t type=(uint8_t)(setup.wValue>>8),idx=(uint8_t)setup.wValue;
 uint16_t n;const uint8_t *p;
 static uint8_t one[2];
 switch(setup.bRequest){
 case 0x05: if(setup.wLength||setup.wIndex||setup.wValue>127)return false;pending_addr=(uint8_t)setup.wValue;ctrl_status();return true;
 case 0x06:
  if(type==1&&idx==0)p=USB_Desc_Device(&n);
  else if(type==2&&idx==0)p=USB_Desc_Config(&n);
  else if(type==3)p=USB_Desc_String(idx,&n);else return false;
  if(!p)return false;if(n>setup.wLength)n=setup.wLength;ctrl_data_in(p,n);return true;
 case 0x08:one[0]=configured;ctrl_data_in(one,1);return true;
 case 0x09:if(setup.wLength||setup.wIndex||setup.wValue>1)return false;
  configured=(uint8_t)setup.wValue;
  if(configured){set_tx(1,USB_EP_TX_NAK);USB_EP_Receive(2);USB_MSC_Reset();}
  else {set_tx(1,USB_EP_TX_NAK);set_rx(2,USB_EP_RX_NAK);}
  ctrl_status();return true;
 case 0x0a:one[0]=0;ctrl_data_in(one,1);return true;
 case 0x0b:if(setup.wValue||setup.wIndex||setup.wLength)return false;ctrl_status();return true;
 case 0x00:one[0]=0;one[1]=0;ctrl_data_in(one,2);return true;
 case 0x01:if(setup.wValue)return false;ctrl_status();return true;
 case 0x03:if(setup.wValue)return false;USB_EP_Stall((uint8_t)setup.wIndex,(setup.wIndex&0x80)!=0);ctrl_status();return true;
 default:return false;
 }
}
static void setup_received(void){
 const uint8_t *p=0;uint16_t n=0;bool ok;
 pma_read(EP0_RX_PMA,ctrl_buf,8);
 setup.bmRequestType=ctrl_buf[0];setup.bRequest=ctrl_buf[1];
 setup.wValue=(uint16_t)ctrl_buf[2]|((uint16_t)ctrl_buf[3]<<8);
 setup.wIndex=(uint16_t)ctrl_buf[4]|((uint16_t)ctrl_buf[5]<<8);
 setup.wLength=(uint16_t)ctrl_buf[6]|((uint16_t)ctrl_buf[7]<<8);
 toggle_reset(0,false);toggle_reset(0,true);
 if((setup.bmRequestType&0x60)==0)ok=std_request();
 else ok=USB_MSC_Control(&setup,&p,&n);
 if(ok&&p){if(n>setup.wLength)n=setup.wLength;ctrl_data_in(p,n);}
 else if(ok)ctrl_status();
 else ctrl_stall();
}
static void bus_reset(void){
 USB->DADDR=USB_DADDR_EF;USB->BTABLE=USB_BTABLE_OFFSET;
 configure_ep(0,USB_EP_CONTROL,64,64);configure_ep(1,USB_EP_BULK,64,0);configure_ep(2,USB_EP_BULK,0,64);
 set_tx(1,USB_EP_TX_NAK);set_rx(2,USB_EP_RX_NAK);configured=0;pending_addr=0xff;ctrl_state=CTRL_IDLE;USB_MSC_Reset();ctrl_setup_ready();
}
bool USB_EP_Send(uint8_t ep,const uint8_t *data,uint16_t len){
 if(ep>2||len>64)return false;
 if(data&&len){if(ep==0)pma_write(EP0_TX_PMA,data,len);else if(ep==1)pma_write(EP1_TX_PMA,data,len);else return false;}
 set_tx_count(ep,len);set_tx(ep,USB_EP_TX_VALID);return true;
}
void USB_EP_Receive(uint8_t ep){if(ep==0||ep==2){set_rx_count(ep,64);set_rx(ep,USB_EP_RX_VALID);}}
void USB_EP_Stall(uint8_t ep,bool in){if(ep<=2){if(in)set_tx(ep,USB_EP_TX_STALL);else set_rx(ep,USB_EP_RX_STALL);}}
void USB_EP_ClearStall(uint8_t ep,bool in){if(ep<=2){toggle_reset(ep,in);if(in)set_tx(ep,USB_EP_TX_NAK);else set_rx(ep,USB_EP_RX_VALID);}}
static void ep0_rx(void){
 uint16_t n=rx_count(0);if(er(0)&USB_EP_SETUP){clear_rx_ctr(0);setup_received();return;}clear_rx_ctr(0);
 if(ctrl_state==CTRL_STATUS_OUT&&n==0){ctrl_state=CTRL_IDLE;ctrl_setup_ready();}else ctrl_setup_ready();
}
static void ep0_tx(void){clear_tx_ctr(0);if(ctrl_state==CTRL_DATA)ctrl_send_next();else if(ctrl_state==CTRL_STATUS_IN){if(pending_addr!=0xff){USB->DADDR=(uint16_t)(USB_DADDR_EF|pending_addr);pending_addr=0xff;}ctrl_state=CTRL_IDLE;ctrl_setup_ready();}else ctrl_setup_ready();}
static void ep2_rx(void){
 uint16_t n=rx_count(2);if(n<=64)pma_read(EP2_RX_PMA,ctrl_buf,n);clear_rx_ctr(2);
 if(configured&&n<=64)USB_MSC_Out(ctrl_buf,n);
 if(configured)USB_EP_Receive(2);
}
static void ep1_tx(void){clear_tx_ctr(1);USB_MSC_InComplete();}
void USB_Device_Init(void){
 USB->CNTR=USB_CNTR_FRES;USB->CNTR=0;USB->ISTR=0;bus_reset();
 USB->CNTR=(uint16_t)(USB_CNTR_CTRM|USB_CNTR_RESETM|USB_CNTR_SUSPM|USB_CNTR_WKUPM|USB_CNTR_PMAOVRM|USB_CNTR_ERRM);USB->BCDR|=USB_BCDR_DPPU;
 NVIC_EnableIRQ(USB_LP_CAN_RX0_IRQn);
}
void USB_Device_IRQHandler(void){
 uint16_t i;
 for(;;){
  i=USB->ISTR;if(!i)break;
  if(i&USB_ISTR_RESET){USB->ISTR=(uint16_t)(i&~USB_ISTR_RESET);bus_reset();continue;}
  if(i&USB_ISTR_CTR){
   uint8_t ep=(uint8_t)(i&USB_ISTR_EP_ID);uint16_t r=er(ep);
   if(r&USB_EP_CTR_RX){if(ep==0)ep0_rx();else if(ep==2)ep2_rx();else clear_rx_ctr(ep);}
   r=er(ep);if(r&USB_EP_CTR_TX){if(ep==0)ep0_tx();else if(ep==1)ep1_tx();else clear_tx_ctr(ep);}
   continue;
  }
  if(i&(USB_ISTR_SUSP|USB_ISTR_WKUP|USB_ISTR_PMAOVR|USB_ISTR_ERR|USB_ISTR_SOF))USB->ISTR=(uint16_t)(i&~(USB_ISTR_SUSP|USB_ISTR_WKUP|USB_ISTR_PMAOVR|USB_ISTR_ERR|USB_ISTR_SOF));else USB->ISTR=0;
 }
}
