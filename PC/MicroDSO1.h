#ifndef CPWINDOW1 
#define CPWINDOW1

#include<lxrad.h>
#include "lcd_ili9341.h"

class CPWindow1:public CPWindow
{
  public:
//lxrad automatic generated block start, don't edit below!
  /*#Controls*/
  CDraw draw1;
  CButton button1;
  CButton button2;
  CButton button3;
  CButton button4;
  CTimer timer1;
  CButton button5;
  CCombo combo1;
  CCombo combo2;
  CCombo combo3;
  CLabel label1;
  CLabel label2;
  CLabel label3;
  CCheckBox checkbox1;
  /*#Events*/
  void _EvOnCreate(CControl * control);
  void _EvOnDestroy(CControl * control);
  void _EvOnShow(CControl * control);
  void button1_EvMouseButtonPress(CControl * control, const uint button, const uint x,const  uint y, const uint state);
  void button1_EvMouseButtonRelease(CControl * control, const uint button, const uint x,const  uint y, const uint state);
  void button2_EvMouseButtonPress(CControl * control, const uint button, const uint x,const  uint y, const uint state);
  void button2_EvMouseButtonRelease(CControl * control, const uint button, const uint x,const  uint y, const uint state);
  void button3_EvMouseButtonPress(CControl * control, const uint button, const uint x,const  uint y, const uint state);
  void button3_EvMouseButtonRelease(CControl * control, const uint button, const uint x,const  uint y, const uint state);
  void button4_EvMouseButtonPress(CControl * control, const uint button, const uint x,const  uint y, const uint state);
  void button4_EvMouseButtonRelease(CControl * control, const uint button, const uint x,const  uint y, const uint state);
  void button5_EvMouseButtonPress(CControl * control, const uint button, const uint x,const  uint y, const uint state);
  void button5_EvMouseButtonRelease(CControl * control, const uint button, const uint x,const  uint y, const uint state);
  void combo1_EvOnComboChange(CControl * control);
  void combo2_EvOnComboChange(CControl * control);
  void combo3_EvOnComboChange(CControl * control);
  void timer1_EvOnTime(CControl * control);

  /*#Others*/
  CPWindow1(void);
//lxrad automatic generated block end, don't edit above!
  int started;
  lcd_ili9341_t lcd;
  void DrawLCD(void);
};

extern CPWindow1 Window1 ;

#endif /*#CPWINDOW1*/

