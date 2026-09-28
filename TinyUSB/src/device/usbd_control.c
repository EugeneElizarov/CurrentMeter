#include "tusb_option.h"
#if CFG_TUD_ENABLED
#include "device/dcd.h"
#include "tusb.h"
#include "device/usbd_pvt.h"

TU_ATTR_WEAK void dcd_edpt0_status_complete(uint8_t rhport, const tusb_control_request_t* request) { (void)rhport; (void)request; }

enum { EDPT_CTRL_OUT=0x00, EDPT_CTRL_IN=0x80 };
typedef struct { tusb_control_request_t request; uint8_t* buffer; uint16_t data_len; uint16_t total_xferred; usbd_control_xfer_cb_t complete_cb; } usbd_control_xfer_t;
static usbd_control_xfer_t _ctrl_xfer;
CFG_TUD_MEM_SECTION static struct { TUD_EPBUF_DEF(buf, CFG_TUD_ENDPOINT0_BUFSIZE); } _ctrl_epbuf;

uint8_t* usbd_get_ctrl_buf(void) { return _ctrl_epbuf.buf; }

static inline bool status_stage_xact(uint8_t rhport, const tusb_control_request_t* request) {
  const uint8_t ep_addr = request->bmRequestType_bit.direction ? EDPT_CTRL_OUT : EDPT_CTRL_IN;
  return usbd_edpt_xfer(rhport, ep_addr, NULL, 0, false);
}
bool tud_control_status(uint8_t rhport, const tusb_control_request_t* request) {
  _ctrl_xfer.request=*request; _ctrl_xfer.buffer=NULL; _ctrl_xfer.total_xferred=0; _ctrl_xfer.data_len=0;
  return status_stage_xact(rhport, request);
}
static bool data_stage_xact(uint8_t rhport) {
  const uint16_t xact_len=tu_min16(_ctrl_xfer.data_len-_ctrl_xfer.total_xferred,CFG_TUD_ENDPOINT0_BUFSIZE);
  uint8_t ep_addr=EDPT_CTRL_OUT;
  if (_ctrl_xfer.request.bmRequestType_bit.direction==TUSB_DIR_IN) {
    ep_addr=EDPT_CTRL_IN;
    if (xact_len && _ctrl_xfer.buffer!=_ctrl_epbuf.buf)
      TU_VERIFY(0==tu_memcpy_s(_ctrl_epbuf.buf,CFG_TUD_ENDPOINT0_BUFSIZE,_ctrl_xfer.buffer,xact_len));
  }
  return usbd_edpt_xfer(rhport,ep_addr,xact_len?_ctrl_epbuf.buf:NULL,xact_len,false);
}
bool tud_control_xfer(uint8_t rhport,const tusb_control_request_t* request,void* buffer,uint16_t len) {
  _ctrl_xfer.request=*request; _ctrl_xfer.buffer=(uint8_t*)buffer; _ctrl_xfer.total_xferred=0U;
  _ctrl_xfer.data_len=tu_min16(len,request->wLength);
  if(request->wLength>0U) { if(_ctrl_xfer.data_len>0U) TU_ASSERT(buffer); TU_ASSERT(data_stage_xact(rhport)); }
  else { TU_ASSERT(status_stage_xact(rhport,request)); }
  return true;
}
void usbd_control_reset(void); void usbd_control_set_request(const tusb_control_request_t* request);
void usbd_control_set_complete_callback(usbd_control_xfer_cb_t fp);
bool usbd_control_xfer_cb(uint8_t rhport,uint8_t ep_addr,xfer_result_t result,uint32_t xferred_bytes);
void usbd_control_reset(void) { tu_varclr(&_ctrl_xfer); }
void usbd_control_set_complete_callback(usbd_control_xfer_cb_t fp) { _ctrl_xfer.complete_cb=fp; }
void usbd_control_set_request(const tusb_control_request_t* request) {
  _ctrl_xfer.request=*request; _ctrl_xfer.buffer=NULL; _ctrl_xfer.total_xferred=0; _ctrl_xfer.data_len=0;
}
bool usbd_control_xfer_cb(uint8_t rhport,uint8_t ep_addr,xfer_result_t result,uint32_t xferred_bytes) {
  (void)result;
  if(tu_edpt_dir(ep_addr)!=_ctrl_xfer.request.bmRequestType_bit.direction) {
    TU_ASSERT(0==xferred_bytes); dcd_edpt0_status_complete(rhport,&_ctrl_xfer.request);
    if(_ctrl_xfer.complete_cb) _ctrl_xfer.complete_cb(rhport,CONTROL_STAGE_ACK,&_ctrl_xfer.request);
    return true;
  }
  if(_ctrl_xfer.request.bmRequestType_bit.direction==TUSB_DIR_OUT) {
    TU_VERIFY(_ctrl_xfer.buffer);
    if(_ctrl_xfer.buffer!=_ctrl_epbuf.buf) memcpy(_ctrl_xfer.buffer,_ctrl_epbuf.buf,xferred_bytes);
    TU_LOG_MEM(CFG_TUD_LOG_LEVEL,_ctrl_xfer.buffer,xferred_bytes,2);
  }
  _ctrl_xfer.total_xferred+=(uint16_t)xferred_bytes; _ctrl_xfer.buffer+=xferred_bytes;
  if((_ctrl_xfer.request.wLength==_ctrl_xfer.total_xferred)||(xferred_bytes<CFG_TUD_ENDPOINT0_BUFSIZE)) {
    bool is_ok=true;
    if(_ctrl_xfer.complete_cb) is_ok=_ctrl_xfer.complete_cb(rhport,CONTROL_STAGE_DATA,&_ctrl_xfer.request);
    if(is_ok) TU_ASSERT(status_stage_xact(rhport,&_ctrl_xfer.request));
    else { dcd_edpt_stall(rhport,EDPT_CTRL_OUT); dcd_edpt_stall(rhport,EDPT_CTRL_IN); }
  } else TU_ASSERT(data_stage_xact(rhport));
  return true;
}
#endif
