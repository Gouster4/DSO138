#include"MicroDSO1.h"
#include"MicroDSO1_d.cc"

#include"src/MicroDSO/MicroDSO.h"
#include "Wrapper.h"
#include"src/MicroDSO/global.h"

CPWindow1 Window1;


//Implementation

//Sel

void
CPWindow1::button1_EvMouseButtonPress(CControl * control, const uint button, const uint x, const uint y, const uint state)
{
 ExternPinWrite (ENCODER_SW, 0);
}

void
CPWindow1::button1_EvMouseButtonRelease(CControl * control, const uint button, const uint x, const uint y, const uint state)
{
 ExternPinWrite (ENCODER_SW, 1);
}

//plus

void
CPWindow1::button2_EvMouseButtonPress(CControl * control, const uint button, const uint x, const uint y, const uint state)
{
 ExternPinWrite (ENCODER_B, 0);
}

void
CPWindow1::button2_EvMouseButtonRelease(CControl * control, const uint button, const uint x, const uint y, const uint state)
{
 ExternPinWrite (ENCODER_B, 1);
}


//minus

void
CPWindow1::button3_EvMouseButtonPress(CControl * control, const uint button, const uint x, const uint y, const uint state)
{
 ExternPinWrite (ENCODER_A, 0);
}

void
CPWindow1::button3_EvMouseButtonRelease(CControl * control, const uint button, const uint x, const uint y, const uint state)
{
 ExternPinWrite (ENCODER_A, 1);
}

//Ok

void
CPWindow1::button4_EvMouseButtonPress(CControl * control, const uint button, const uint x, const uint y, const uint state)
{
 ExternPinWrite (BTN4, 0);
}

void
CPWindow1::button4_EvMouseButtonRelease(CControl * control, const uint button, const uint x, const uint y, const uint state)
{
 ExternPinWrite (BTN4, 1);
}

void
CPWindow1::_EvOnCreate(CControl * control)
{
 //code here:)
 mprint (lxT ("_EvOnCreate\n"));
 started = 0;
 ExternPinWrite (ENCODER_SW, 1);
 ExternPinWrite (ENCODER_A, 1);
 ExternPinWrite (ENCODER_B, 1);
 ExternPinWrite (BTN4, 1);

 adc[CPLSEL_channel] = 4095;
 adc[VSENSSEL1_channel] = 0;
 adc[VSENSSEL2_channel] = 4095;
}

void
CPWindow1::_EvOnDestroy(CControl * control)
{
 //code here:)
 mprint (lxT ("_EvOnDestroy\n"));
}

void
CPWindow1::_EvOnShow(CControl * control)
{
 if (!started)
  {
   timer1.SetRunState (1);
  }
}

//Reset

void
CPWindow1::button5_EvMouseButtonPress(CControl * control, const uint button, const uint x, const uint y, const uint state)
{
 started = 0;
}

void
CPWindow1::button5_EvMouseButtonRelease(CControl * control, const uint button, const uint x, const uint y, const uint state)
{
 //code here:)
 mprint (lxT ("button5_EvMouseButtonRelease\n"));
}

//CPL

void
CPWindow1::combo1_EvOnComboChange(CControl * control)
{
 if (!combo1.GetText ().Cmp ("GND"))
  {
   adc[CPLSEL_channel] = 0;
  }
 else if (!combo1.GetText ().Cmp ("AC"))
  {
   adc[CPLSEL_channel] = 1500;
  }
 else
  {
   adc[CPLSEL_channel] = 4095;
  }
}

//SEN1

void
CPWindow1::combo2_EvOnComboChange(CControl * control)
{
 if (!combo2.GetText ().Cmp ("1V"))
  {
   adc[VSENSSEL1_channel] = 0;
  }
 else if (!combo2.GetText ().Cmp ("0.1V"))
  {
   adc[VSENSSEL1_channel] = 1500;
  }
 else
  {
   adc[VSENSSEL1_channel] = 4095;
  }
}

//SEN2

void
CPWindow1::combo3_EvOnComboChange(CControl * control)
{
 if (!combo3.GetText ().Cmp ("X5"))
  {
   adc[VSENSSEL2_channel] = 0;
  }
 else if (!combo3.GetText ().Cmp ("X2"))
  {
   adc[VSENSSEL2_channel] = 1500;
  }
 else
  {
   adc[VSENSSEL2_channel] = 4095;
  }
}

void
CPWindow1::timer1_EvOnTime(CControl * control)
{
 if (!started)
  {
   lcd_ili9341_init (&lcd);
   MicroDSO_Setup ();
   started = 1;
  }
 else
  {
   MicroDSO_Loop ();
   DrawLCD ();
  }

}

void
CPWindow1::DrawLCD(void)
{
 draw1.Canvas.Init ();
 lcd_ili9341_draw (&lcd, &draw1.Canvas, 0, 0, 240, 320, 1);
 draw1.Canvas.End ();
}