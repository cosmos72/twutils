/*
    $Id: kcalc.cpp,v 1.18 1999/01/12 13:03:26 kulow Exp $

    kCalculator, a simple scientific calculator for KDE

    Copyright (C) 1996 Bernd Johannes Wuebben wuebben@math.cornell.edu

    Copyright (C) 2001 Massimiliano Ghilardi  max@linuz.sns.it

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "kcalc.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "configdlg.h"
#include "version.h"

#define CHECK_PTR(p) assert(p && p->Id)

extern last_input_type last_input;
extern item_contents display_data;
extern num_base current_base;
// KApplication           *mykapp;

TList<CALCAMNT> temp_stack;
TMenu *Menu;
TW *dpy;

enum codes {
  twKalc = 1,
  kDisplay,
  Hex,
  Dec,
  Oct,
  Bin,
  Deg,
  Rad,
  Gra,
  Hyp,
  Inv,
  _A_,
  Sin,
  neg,
  _B_,
  Cos,
  _1x,
  _C_,
  Tan,
  _xf,
  _D_,
  Log,
  x_2,
  _E_,
  Ln_,
  x_y,
  _F_,
  EE_,
  MR_,
  Mpm,
  MC_,
  clr,
  AC_,
  _7_,
  _8_,
  _9_,
  lpa,
  rpa,
  And_,
  _4_,
  _5_,
  _6_,
  _X_,
  frc,
  Or_,
  _1_,
  _2_,
  _3_,
  pls,
  min,
  Lsh,
  dot,
  _0_,
  eql,
  prc,
  Cmp,
  mod
};

/*
 * warning: except for isToggle() gadgets, libTw sends TGadgetEvent messages
 * when a gadget is UNPRESSED, not when it is PRESSED!. so we must reverse the
 * usage of isOn() in the code.
 */
static inline bool isUp(TEvent *E) {
  return !(E->gadgetEvent()->flags() & TW_GADGETFL_PRESSED);
}
static inline bool isDown(TEvent *E) {
  return !!(E->gadgetEvent()->flags() & TW_GADGETFL_PRESSED);
}

QtCalculator ::QtCalculator(const char *name) : TWindow(Menu, name) {
  int u = 0;

  readSettings();

  /* set the default listener: */
  dpy->setDefaultListener(TDL(this, &QtCalculator::defaultListener));
  /*
   * check Tw/Tw++.h for the 'TDL(...)' macro: the above line expands to
   * dpy->setDefaultListener(
   *      new TDefaultListenerNonStatic(
   *          (TEmpty *)this,
   *          (void (TEmpty::*)(TMsg *, void *))&QtCalculator::defaultListener
   *      )
   * );
   *
   * that tells libTw to call `this->defaultListener(TMsg *, void *)'
   * when the user generates an event which is not captured by any specific
   * listener.
   */

  // create help button
  //
  TGadget *pb;

  pb = new TGadget(this, 8, 1, " twKalc ", twKalc);
  pb->setXY(1, 1);
  dpy->addListener(new TGadgetEvent(this, twKalc),
                   TL(this, &QtCalculator::configclicked));
  /*
   * check Tw/Tw++.h for the 'TL(...)' macro: the above line expands to
   * dpy->addListener(
   *      new TGadgetEvent(this, twKalc),
   *      new TListenerNonStatic(
   *          (TEmpty *)this,
   *          (void (TEmpty::*)(TEvent *, void *))&QtCalculator::configclicked
   *      )
   * );
   *
   * that tells libTw to call `this->configclicked()' when the user clicks on
   * twKalc gadget.
   */

  // Create the display

  calc_display = new TGadget(this, 24, 1, "                        ", kDisplay,
                             TW_GADGETFL_TEXT_DEFCOL, 0, TCOL(tblack, twhite),
                             TCOL(thigh | twhite, twhite));
  calc_display->setXY(12, 1);

  dpy->addListener(new TGadgetEvent(this, kDisplay),
                   TLS(&QtCalculator::display_clicked));
  /*
   * QtCalculator::display_clicked() is a static method...
   * use TLS() instead of TL(). the above line expands to:
   *
   * dpy->addListener(
   *      new TGadgetEvent(this, twDisplay),
   *      new TListener(
   *          (void (*)(TEvent *, void *))&QtCalculator::configclicked
   *      )
   * );
   */

  /* user_wants_to_paste() and someclient_wants_selection() are static too... */
  dpy->addListener(new TSelectionEvent(this),
                   TLS(&QtCalculator::user_wants_to_paste));
  dpy->addListener(new TSelectionRequestEvent(),
                   TLS(&QtCalculator::someclient_wants_selection));
  dpy->addListener(new TClipboard(),
                   TL(this, &QtCalculator::notified_selection));

  this->setSize(38, 18);
  /* set text color and selection color to same value: */
  this->setColors(0x60, 0, 0, 0, 0, 0, TCOL(tgreen, twhite),
                  TCOL(tgreen, twhite), 0, 0);
  this->gotoXY(10, 0);
  /* default charset is IBM-437.
   * Use it to draw some lines around the displayed number */
  this->writeCharset(
      "\xDA\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4"
      "\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xBF");
  this->gotoXY(10, 1);
  this->writeCharset("\xB3                          \xB3");
  this->gotoXY(10, 2);
  this->writeCharset(
      "\xC0\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4"
      "\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xD9");

  /*
   * you must create a new TListener(...) for each event:
   * using the same `TL(&QtCalculator::quitCalc, this)'
   * for all the three calls below would not work.
   */
  dpy->addListener(new TKeyEvent(this, 'q', TW_KBD_ALT_FL),
                   TL(this, &QtCalculator::quitCalc));
  dpy->addListener(new TKeyEvent(this, 'x', TW_KBD_ALT_FL),
                   TL(this, &QtCalculator::quitCalc));
  dpy->addListener(new TGadgetEvent(this, 0 /* window close */),
                   TL(this, &QtCalculator::quitCalc));

  //
  // Create Number Base Button Group
  //

  TGroup *base_group = new TGroup();

  basebutton[0] = new TGadget(this, 3, 1, "Hex", Hex, TW_GADGETFL_TOGGLE);
  basebutton[0]->setXY(1, 3);
  dpy->addListener(new TGadgetEvent(this, Hex),
                   TL(this, &QtCalculator::Hex_Toggled));
  /*
   * libTw has no way to say that a gadget should be activated when hitting a
   * certain key, so we must do it manually: let's bind ALT + `h' to
   * this->Hex_Selected() :
   */
  dpy->addListener(new TKeyEvent(this, 'h', TW_KBD_ALT_FL),
                   TL(this, &QtCalculator::Hex_Selected));

  basebutton[1] = new TGadget(this, 3, 1, "Dec", Dec, TW_GADGETFL_TOGGLE);
  basebutton[1]->setXY(5, 3);
  dpy->addListener(new TGadgetEvent(this, Dec),
                   TL(this, &QtCalculator::Dec_Toggled));
  dpy->addListener(new TKeyEvent(this, 'e', TW_KBD_ALT_FL),
                   TL(this, &QtCalculator::Dec_Selected));

  basebutton[2] = new TGadget(this, 3, 1, "Oct", Oct, TW_GADGETFL_TOGGLE);
  basebutton[2]->setXY(9, 3);
  dpy->addListener(new TGadgetEvent(this, Oct),
                   TL(this, &QtCalculator::Oct_Toggled));
  dpy->addListener(new TKeyEvent(this, 'o', TW_KBD_ALT_FL),
                   TL(this, &QtCalculator::Oct_Selected));

  basebutton[3] = new TGadget(this, 3, 1, "Bin", Bin, TW_GADGETFL_TOGGLE);
  basebutton[3]->setXY(13, 3);
  dpy->addListener(new TGadgetEvent(this, Bin),
                   TL(this, &QtCalculator::Bin_Toggled));
  dpy->addListener(new TKeyEvent(this, 'b', TW_KBD_ALT_FL),
                   TL(this, &QtCalculator::Bin_Selected));

  for (u = 0; u < 4; u++) base_group->insertGadget(basebutton[u]);

  basebutton[1]->setOn(ttrue);

  TGroup *angle_group = new TGroup();

  anglebutton[0] = new TGadget(this, 3, 1, "Deg", Deg, TW_GADGETFL_TOGGLE);
  anglebutton[0]->setXY(25, 3);
  dpy->addListener(new TGadgetEvent(this, Deg),
                   TL(this, &QtCalculator::Deg_Toggled));
  dpy->addListener(new TKeyEvent(this, 'd', TW_KBD_ALT_FL),
                   TL(this, &QtCalculator::Deg_Selected));

  anglebutton[1] = new TGadget(this, 3, 1, "Rad", Rad, TW_GADGETFL_TOGGLE);
  anglebutton[1]->setXY(29, 3);
  dpy->addListener(new TGadgetEvent(this, Rad),
                   TL(this, &QtCalculator::Rad_Toggled));
  dpy->addListener(new TKeyEvent(this, 'r', TW_KBD_ALT_FL),
                   TL(this, &QtCalculator::Rad_Selected));

  anglebutton[2] = new TGadget(this, 3, 1, "Gra", Gra, TW_GADGETFL_TOGGLE);
  anglebutton[2]->setXY(33, 3);
  dpy->addListener(new TGadgetEvent(this, Gra),
                   TL(this, &QtCalculator::Gra_Toggled));
  dpy->addListener(new TKeyEvent(this, 'g', TW_KBD_ALT_FL),
                   TL(this, &QtCalculator::Gra_Selected));

  for (u = 0; u < 3; u++) angle_group->insertGadget(anglebutton[u]);

  anglebutton[0]->setOn(ttrue);

  //
  //  Create Calculator Buttons
  //

  pbhyp = new TButton(this, 3, 1, "Hyp", Hyp);
  pbhyp->setXY(1, 5);
  /*
   * we exploit a libTw feature (limitation?) :
   * while TW_GADGETFL_TOGGLE buttons/gadgets generate events
   * both when pressed and when unpressed,
   * normal buttons/gadgets generate *one* event only *after*
   * a press/unpress cycle (i.e. a click).
   */
  dpy->addListener(new TGadgetEvent(this, Hyp),
                   TL(this, &QtCalculator::EnterHyp));

  pbinv = new TButton(this, 3, 1, "Inv", Inv);
  pbinv->setXY(5, 5);
  dpy->addListener(new TGadgetEvent(this, Inv),
                   TL(this, &QtCalculator::SetInverse));

  pbA = new TButton(this, 3, 1, " A ", _A_);
  pbA->setXY(9, 5);
  dpy->addListener(new TGadgetEvent(this, _A_),
                   TL(this, &QtCalculator::buttonA));

  pbSin = new TButton(this, 3, 1, "Sin", Sin);
  pbSin->setXY(1, 7);
  dpy->addListener(new TGadgetEvent(this, Sin),
                   TL(this, &QtCalculator::ExecSin));

  pbplusminus = new TButton(this, 3, 1, "+/-", neg);
  pbplusminus->setXY(5, 7);
  dpy->addListener(new TGadgetEvent(this, neg),
                   TL(this, &QtCalculator::EnterNegate));

  pbB = new TButton(this, 3, 1, " B ", _B_);
  pbB->setXY(9, 7);
  dpy->addListener(new TGadgetEvent(this, _B_),
                   TL(this, &QtCalculator::buttonB));

  pbCos = new TButton(this, 3, 1, "Cos", Cos);
  pbCos->setXY(1, 9);
  dpy->addListener(new TGadgetEvent(this, Cos),
                   TL(this, &QtCalculator::ExecCos));

  pbreci = new TButton(this, 3, 1, "1/x", _1x);
  pbreci->setXY(5, 9);
  dpy->addListener(new TGadgetEvent(this, _1x),
                   TL(this, &QtCalculator::EnterRecip));

  pbC = new TButton(this, 3, 1, " C ", _C_);
  pbC->setXY(9, 9);
  dpy->addListener(new TGadgetEvent(this, _C_),
                   TL(this, &QtCalculator::buttonC));

  pbTan = new TButton(this, 3, 1, "Tan", Tan);
  pbTan->setXY(1, 11);
  dpy->addListener(new TGadgetEvent(this, Tan),
                   TL(this, &QtCalculator::ExecTan));

  pbfactorial = new TButton(this, 3, 1, " x!", _xf);
  pbfactorial->setXY(5, 11);
  dpy->addListener(new TGadgetEvent(this, _xf),
                   TL(this, &QtCalculator::EnterFactorial));

  pbD = new TButton(this, 3, 1, " D ", _D_);
  pbD->setXY(9, 11);
  dpy->addListener(new TGadgetEvent(this, _D_),
                   TL(this, &QtCalculator::buttonD));

  pblog = new TButton(this, 3, 1, "Log", Log);
  pblog->setXY(1, 13);
  dpy->addListener(new TGadgetEvent(this, Log),
                   TL(this, &QtCalculator::EnterLogr));

  pbsquare = new TButton(this, 3, 1, "x^2", x_2);
  pbsquare->setXY(5, 13);
  dpy->addListener(new TGadgetEvent(this, x_2),
                   TL(this, &QtCalculator::EnterSquare));

  pbE = new TButton(this, 3, 1, " E ", _E_);
  pbE->setXY(9, 13);
  dpy->addListener(new TGadgetEvent(this, _E_),
                   TL(this, &QtCalculator::buttonE));

  pbln = new TButton(this, 3, 1, "Ln ", Ln_);
  pbln->setXY(1, 15);
  dpy->addListener(new TGadgetEvent(this, Ln_),
                   TL(this, &QtCalculator::EnterLogn));

  pbpower = new TButton(this, 3, 1, "x^y", x_y);
  pbpower->setXY(5, 15);
  dpy->addListener(new TGadgetEvent(this, x_y), TL(this, &QtCalculator::Power));

  pbF = new TButton(this, 3, 1, " F ", _F_);
  pbF->setXY(9, 15);
  dpy->addListener(new TGadgetEvent(this, _F_),
                   TL(this, &QtCalculator::buttonF));

  //////////////////////////////////////////////////////////////////////
  //
  //

  pbEE = new TButton(this, 3, 1, "EE ", EE_);
  pbEE->setXY(13, 5);
  dpy->addListener(new TGadgetEvent(this, EE_), TL(this, &QtCalculator::EE));

  pbMR = new TButton(this, 3, 1, "MR ", MR_);
  pbMR->setXY(17, 5);
  dpy->addListener(new TGadgetEvent(this, MR_), TL(this, &QtCalculator::MR));

  pbMplusminus = new TButton(this, 3, 1, "M+-", Mpm);
  pbMplusminus->setXY(21, 5);
  dpy->addListener(new TGadgetEvent(this, Mpm),
                   TL(this, &QtCalculator::Mplusminus));

  pbMC = new TButton(this, 3, 1, "MC ", MC_);
  pbMC->setXY(25, 5);
  dpy->addListener(new TGadgetEvent(this, MC_), TL(this, &QtCalculator::MC));

  pbClear = new TButton(this, 3, 1, " C ", clr);
  pbClear->setXY(29, 5);
  dpy->addListener(new TGadgetEvent(this, clr), TL(this, &QtCalculator::Clear));

  pbAC = new TButton(this, 3, 1, "AC ", AC_);
  pbAC->setXY(33, 5);
  dpy->addListener(new TGadgetEvent(this, AC_),
                   TL(this, &QtCalculator::ClearAll));

  //////////////////////////////////////////////////////////////////////
  //
  //

  pb7 = new TButton(this, 3, 1, " 7 ", _7_);
  pb7->setXY(13, 7);
  dpy->addListener(new TGadgetEvent(this, _7_),
                   TL(this, &QtCalculator::button7));

  pb8 = new TButton(this, 3, 1, " 8 ", _8_);
  pb8->setXY(17, 7);
  dpy->addListener(new TGadgetEvent(this, _8_),
                   TL(this, &QtCalculator::button8));

  pb9 = new TButton(this, 3, 1, " 9 ", _9_);
  pb9->setXY(21, 7);
  dpy->addListener(new TGadgetEvent(this, _9_),
                   TL(this, &QtCalculator::button9));

  pbparenopen = new TButton(this, 3, 1, " ( ", lpa);
  pbparenopen->setXY(25, 7);
  dpy->addListener(new TGadgetEvent(this, lpa),
                   TL(this, &QtCalculator::EnterOpenParen));

  pbparenclose = new TButton(this, 3, 1, " ) ", rpa);
  pbparenclose->setXY(29, 7);
  dpy->addListener(new TGadgetEvent(this, rpa),
                   TL(this, &QtCalculator::EnterCloseParen));

  pband = new TButton(this, 3, 1, "And", And_);
  pband->setXY(33, 7);
  dpy->addListener(new TGadgetEvent(this, And_), TL(this, &QtCalculator::And));

  //////////////////////////////////////////////////////////////////////
  //
  //

  pb4 = new TButton(this, 3, 1, " 4 ", _4_);
  pb4->setXY(13, 9);
  dpy->addListener(new TGadgetEvent(this, _4_),
                   TL(this, &QtCalculator::button4));

  pb5 = new TButton(this, 3, 1, " 5 ", _5_);
  pb5->setXY(17, 9);
  dpy->addListener(new TGadgetEvent(this, _5_),
                   TL(this, &QtCalculator::button5));

  pb6 = new TButton(this, 3, 1, " 6 ", _6_);
  pb6->setXY(21, 9);
  dpy->addListener(new TGadgetEvent(this, _6_),
                   TL(this, &QtCalculator::button6));

  pbX = new TButton(this, 3, 1, " X ", _X_);
  pbX->setXY(25, 9);
  dpy->addListener(new TGadgetEvent(this, _X_),
                   TL(this, &QtCalculator::Multiply));

  pbdivision = new TButton(this, 3, 1, " / ", frc);
  pbdivision->setXY(29, 9);
  dpy->addListener(new TGadgetEvent(this, frc),
                   TL(this, &QtCalculator::Divide));

  pbor = new TButton(this, 3, 1, "Or ", Or_);
  pbor->setXY(33, 9);
  dpy->addListener(new TGadgetEvent(this, Or_), TL(this, &QtCalculator::Or));

  /////////////////////////////////////////////////////////////////////////////
  //
  //

  pb1 = new TButton(this, 3, 1, " 1 ", _1_);
  pb1->setXY(13, 11);
  dpy->addListener(new TGadgetEvent(this, _1_),
                   TL(this, &QtCalculator::button1));

  pb2 = new TButton(this, 3, 1, " 2 ", _2_);
  pb2->setXY(17, 11);
  dpy->addListener(new TGadgetEvent(this, _2_),
                   TL(this, &QtCalculator::button2));

  pb3 = new TButton(this, 3, 1, " 3 ", _3_);
  pb3->setXY(21, 11);
  dpy->addListener(new TGadgetEvent(this, _3_),
                   TL(this, &QtCalculator::button3));

  pbplus = new TButton(this, 3, 1, " + ", pls);
  pbplus->setXY(25, 11);
  dpy->addListener(new TGadgetEvent(this, pls), TL(this, &QtCalculator::Plus));

  pbminus = new TButton(this, 3, 1, " - ", min);
  pbminus->setXY(29, 11);
  dpy->addListener(new TGadgetEvent(this, min), TL(this, &QtCalculator::Minus));

  pbshift = new TButton(this, 3, 1, "Lsh", Lsh);
  pbshift->setXY(33, 11);
  dpy->addListener(new TGadgetEvent(this, Lsh), TL(this, &QtCalculator::Shift));

  ///////////////////////////////////////////////////////////////////////////
  //
  //

  pbperiod = new TButton(this, 3, 1, " . ", dot);
  pbperiod->setXY(13, 13);
  dpy->addListener(new TGadgetEvent(this, dot),
                   TL(this, &QtCalculator::EnterDecimal));

  pb0 = new TButton(this, 3, 1, " 0 ", _0_);
  pb0->setXY(17, 13);
  dpy->addListener(new TGadgetEvent(this, _0_),
                   TL(this, &QtCalculator::button0));

  pbequal = new TButton(this, 3, 1, " = ", eql);
  pbequal->setXY(21, 13);
  dpy->addListener(new TGadgetEvent(this, eql),
                   TL(this, &QtCalculator::EnterEqual));

  pbpercent = new TButton(this, 3, 1, " % ", prc);
  pbpercent->setXY(25, 13);
  dpy->addListener(new TGadgetEvent(this, prc),
                   TL(this, &QtCalculator::EnterPercent));

  pbnegate = new TButton(this, 3, 1, "Cmp", Cmp);
  pbnegate->setXY(29, 13);
  dpy->addListener(new TGadgetEvent(this, Cmp),
                   TL(this, &QtCalculator::EnterNotCmp));

  pbmod = new TButton(this, 3, 1, "Mod", mod);
  pbmod->setXY(33, 13);
  dpy->addListener(new TGadgetEvent(this, mod), TL(this, &QtCalculator::Mod));

  statusINVLabel = new TGadget(this, 4, 1, "NORM", 0, TW_GADGETFL_TEXT_DEFCOL,
                               0, TCOL(tblack, twhite));
  statusINVLabel->setXY(1, 17);

  statusHYPLabel = new TGadget(this, 4, 1, "HYP ", 0, TW_GADGETFL_TEXT_DEFCOL,
                               0, TCOL(tblack, twhite));
  statusHYPLabel->setXY(7, 17);

  statusERRORLabel =
      new TGadget(this, 20, 1, "                    ", 0,
                  TW_GADGETFL_TEXT_DEFCOL, 0, TCOL(tblack, twhite));
  statusERRORLabel->setXY(13, 17);

  set_colors();
  set_precision();
  set_style();

  InitializeCalculator();
}

void QtCalculator::quit() { exit(0); }

void QtCalculator::helpclicked() {
  //  mykapp->invokeHTMLHelp("","");
}

void QtCalculator::defaultListener(TMsg *M, void *Arg) {
  if (M->type() == TW_MSG_WIDGET_KEY) keyPressEvent(M->event()->keyEvent());
}

void QtCalculator::keyPressEvent(TKeyEvent *e) {
  switch (e->key()) {
    case TW_F1:
      helpclicked();
      break;
    case TW_F2:
      configclicked();
      break;
    case TW_F3:
      if (kcalcdefaults.style == 0)
        kcalcdefaults.style = 1;
      else
        kcalcdefaults.style = 0;
      set_style();
      break;
    case TW_Up:
    case TW_KP_Up:
      temp_stack_prev();
      break;
    case TW_Down:
    case TW_KP_Down:
      temp_stack_next();
      break;

    case TW_BackSpace:
    case TW_Next:
    case TW_Delete:
      key_pressed = ttrue;
      pbAC->setOn(ttrue);
      break;
    case TW_Escape:
    case TW_Prior:
      key_pressed = ttrue;
      pbClear->setOn(ttrue);
      break;

    case 'H':
    case 'h':
      key_pressed = ttrue;
      pbhyp->setOn(ttrue);
      break;
    case 'I':
    case 'i':
      key_pressed = ttrue;
      pbinv->setOn(ttrue);
      break;
    case 'A':
    case 'a':
      key_pressed = ttrue;
      pbA->setOn(ttrue);

      break;
    case 'E':
    case 'e':
      key_pressed = ttrue;
      if (current_base == NB_HEX)
        pbE->setOn(ttrue);
      else
        pbEE->setOn(ttrue);
      break;
    case 'S':
    case 's':
      key_pressed = ttrue;
      pbSin->setOn(ttrue);
      break;
    case '\\':
      key_pressed = ttrue;
      pbplusminus->setOn(ttrue);
      break;
    case 'B':
    case 'b':
      key_pressed = ttrue;
      pbB->setOn(ttrue);
      break;
    case '7':
    case TW_KP_7:
      key_pressed = ttrue;
      pb7->setOn(ttrue);
      break;
    case '8':
    case TW_KP_8:
      key_pressed = ttrue;
      pb8->setOn(ttrue);
      break;
    case '9':
    case TW_KP_9:
      key_pressed = ttrue;
      pb9->setOn(ttrue);
      break;
    case '(':
      key_pressed = ttrue;
      pbparenopen->setOn(ttrue);
      break;
    case ')':
      key_pressed = ttrue;
      pbparenclose->setOn(ttrue);
      break;
    case '&':
      key_pressed = ttrue;
      pband->setOn(ttrue);
      break;
    case 'C':
    case 'c':
      key_pressed = ttrue;
      if (current_base == NB_HEX)
        pbC->setOn(ttrue);
      else
        pbCos->setOn(ttrue);
      break;
    case '4':
    case TW_KP_4:
      key_pressed = ttrue;
      pb4->setOn(ttrue);
      break;
    case '5':
    case TW_KP_5:
      key_pressed = ttrue;
      pb5->setOn(ttrue);
      break;
    case '6':
    case TW_KP_6:
      key_pressed = ttrue;
      pb6->setOn(ttrue);
      break;
    case '*':
    case TW_KP_Multiply:
      key_pressed = ttrue;
      pbX->setOn(ttrue);
      break;
    case '/':
    case TW_KP_Divide:
      key_pressed = ttrue;
      pbdivision->setOn(ttrue);
      break;
    case 'O':
    case 'o':
    case '|':
      key_pressed = ttrue;
      pbor->setOn(ttrue);
      break;
    case 'T':
    case 't':
      key_pressed = ttrue;
      pbTan->setOn(ttrue);
      break;
    case '!':
      key_pressed = ttrue;
      pbfactorial->setOn(ttrue);
      break;
    case 'D':
    case 'd':
      key_pressed = ttrue;
      if (kcalcdefaults.style == 0)
        pbD->setOn(ttrue);  // trig mode
      else
        pblog->setOn(ttrue);  // stat mode
      break;
    case '1':
    case TW_KP_1:
      key_pressed = ttrue;
      pb1->setOn(ttrue);
      break;
    case '2':
    case TW_KP_2:
      key_pressed = ttrue;
      pb2->setOn(ttrue);
      break;
    case '3':
    case TW_KP_3:
      key_pressed = ttrue;
      pb3->setOn(ttrue);
      break;
    case '+':
    case TW_KP_Add:
      key_pressed = ttrue;
      pbplus->setOn(ttrue);
      break;
    case '-':
    case TW_KP_Subtract:
      key_pressed = ttrue;
      pbminus->setOn(ttrue);
      break;
    case '<':
      key_pressed = ttrue;
      pbshift->setOn(ttrue);
      break;
    case 'N':
    case 'n':
      key_pressed = ttrue;
      pbln->setOn(ttrue);
      break;
    case 'L':
    case 'l':
      key_pressed = ttrue;
      pblog->setOn(ttrue);
      break;
    case '@':
      key_pressed = ttrue;
      pbpower->setOn(ttrue);
      break;
    case 'F':
    case 'f':
      key_pressed = ttrue;
      pbF->setOn(ttrue);
      break;
    case '.':
    case ',':
    case TW_KP_Decimal:
      key_pressed = ttrue;
      pbperiod->setOn(ttrue);
      break;
    case '0':
    case TW_KP_0:
      key_pressed = ttrue;
      pb0->setOn(ttrue);
      break;
    case TW_Return:
    case TW_KP_Enter:
    case '=':
      key_pressed = ttrue;
      pbequal->setOn(ttrue);
      break;
    case '%':
      key_pressed = ttrue;
      pbpercent->setOn(ttrue);
      break;
    case '~':
      key_pressed = ttrue;
      pbnegate->setOn(ttrue);
      break;
    case ':':
      key_pressed = ttrue;
      pbmod->setOn(ttrue);
      break;
    case '{':
      key_pressed = ttrue;
      pbsquare->setOn(ttrue);
      break;
    case 'R':
    case 'r':
      key_pressed = ttrue;
      pbreci->setOn(ttrue);
      break;
  }

  /* libTw has no keyRelease events, so.... */
  if (key_pressed) {
    dpy->flush();
    usleep(20000);
    if (!dpy->peekMsg()) usleep(60000);
    keyReleaseEvent(e);
  }
}

void QtCalculator::keyReleaseEvent(TKeyEvent *e) {
  switch (e->key()) {
    case TW_BackSpace:
    case TW_Delete:
    case TW_Next:
      key_pressed = tfalse;
      pbAC->setOn(tfalse);
      break;
    case TW_Escape:
    case TW_Prior:
      key_pressed = tfalse;
      pbClear->setOn(tfalse);
      break;

    case 'H':
    case 'h':
      key_pressed = tfalse;
      pbhyp->setOn(tfalse);
      break;
    case 'I':
    case 'i':
      key_pressed = tfalse;
      pbinv->setOn(tfalse);
      break;
    case 'A':
    case 'a':
      key_pressed = tfalse;
      pbA->setOn(tfalse);
      break;
    case 'E':
    case 'e':
      key_pressed = tfalse;
      if (current_base == NB_HEX)
        pbE->setOn(tfalse);
      else
        pbEE->setOn(tfalse);
      break;
    case 'S':
    case 's':
      key_pressed = tfalse;
      pbSin->setOn(tfalse);
      break;
    case '\\':
      key_pressed = tfalse;
      pbplusminus->setOn(tfalse);
      break;
    case 'B':
    case 'b':
      key_pressed = tfalse;
      pbB->setOn(tfalse);
      break;
    case '7':
    case TW_KP_7:
      key_pressed = tfalse;
      pb7->setOn(tfalse);
      break;
    case '8':
    case TW_KP_8:
      key_pressed = tfalse;
      pb8->setOn(tfalse);
      break;
    case '9':
    case TW_KP_9:
      key_pressed = tfalse;
      pb9->setOn(tfalse);
      break;
    case '(':
      key_pressed = tfalse;
      pbparenopen->setOn(tfalse);
      break;
    case ')':
      key_pressed = tfalse;
      pbparenclose->setOn(tfalse);
      break;
    case '&':
      key_pressed = tfalse;
      pband->setOn(tfalse);
      break;
    case 'C':
    case 'c':
      key_pressed = tfalse;
      if (current_base == NB_HEX)
        pbC->setOn(tfalse);
      else
        pbCos->setOn(tfalse);
      break;
    case '4':
    case TW_KP_4:
      key_pressed = tfalse;
      pb4->setOn(tfalse);
      break;
    case '5':
    case TW_KP_5:
      key_pressed = tfalse;
      pb5->setOn(tfalse);
      break;
    case '6':
    case TW_KP_6:
      key_pressed = tfalse;
      pb6->setOn(tfalse);
      break;
    case '*':
    case TW_KP_Multiply:
      key_pressed = tfalse;
      pbX->setOn(tfalse);
      break;
    case '/':
    case TW_KP_Divide:
      key_pressed = tfalse;
      pbdivision->setOn(tfalse);
      break;
    case 'O':
    case 'o':
    case '|':
      key_pressed = tfalse;
      pbor->setOn(tfalse);
      break;
    case 'T':
    case 't':
      key_pressed = tfalse;
      pbTan->setOn(tfalse);
      break;
    case '!':
      key_pressed = tfalse;
      pbfactorial->setOn(tfalse);
      break;
    case 'D':
    case 'd':
      key_pressed = tfalse;
      if (kcalcdefaults.style == 0)
        pbD->setOn(tfalse);  // trig mode
      else
        pblog->setOn(tfalse);  // stat mode
      break;
    case '1':
    case TW_KP_1:
      key_pressed = tfalse;
      pb1->setOn(tfalse);
      break;
    case '2':
    case TW_KP_2:
      key_pressed = tfalse;
      pb2->setOn(tfalse);
      break;
    case '3':
    case TW_KP_3:
      key_pressed = tfalse;
      pb3->setOn(tfalse);
      break;
    case '+':
    case TW_KP_Add:
      key_pressed = tfalse;
      pbplus->setOn(tfalse);
      break;
    case '-':
    case TW_KP_Subtract:
      key_pressed = tfalse;
      pbminus->setOn(tfalse);
      break;
    case '<':
      key_pressed = tfalse;
      pbshift->setOn(tfalse);
      break;
    case 'N':
    case 'n':
      key_pressed = tfalse;
      pbln->setOn(tfalse);
      break;
    case 'L':
    case 'l':
      key_pressed = tfalse;
      pblog->setOn(tfalse);
      break;
    case '@':
      key_pressed = tfalse;
      pbpower->setOn(tfalse);
      break;
    case 'F':
    case 'f':
      key_pressed = tfalse;
      pbF->setOn(tfalse);
      break;
    case '.':
    case ',':
    case TW_KP_Decimal:
      key_pressed = tfalse;
      pbperiod->setOn(tfalse);
      break;
    case '0':
    case TW_KP_0:
      key_pressed = tfalse;
      pb0->setOn(tfalse);
      break;
    case TW_Return:
    case TW_KP_Enter:
    case '=':
      key_pressed = tfalse;
      pbequal->setOn(tfalse);
      break;
    case '%':
      key_pressed = tfalse;
      pbpercent->setOn(tfalse);
      break;
    case '~':
      key_pressed = tfalse;
      pbnegate->setOn(tfalse);
      break;
    case ':':
      key_pressed = tfalse;
      pbmod->setOn(tfalse);
      break;
    case '{':
      key_pressed = tfalse;
      pbsquare->setOn(tfalse);
      break;
    case 'R':
    case 'r':
      key_pressed = tfalse;
      pbreci->setOn(tfalse);
      break;
  }
}

/*
 * handle keyboard binds to activate buttons:
 * no need to release the other buttons, the exclusion mechanism
 * is handled by server since they are in the same group.
 */
void QtCalculator::Hex_Selected() { basebutton[0]->setOn(ttrue); }
void QtCalculator::Dec_Selected() { basebutton[1]->setOn(ttrue); }
void QtCalculator::Oct_Selected() { basebutton[2]->setOn(ttrue); }
void QtCalculator::Bin_Selected() { basebutton[3]->setOn(ttrue); }

void QtCalculator::Deg_Selected() { anglebutton[0]->setOn(ttrue); }
void QtCalculator::Rad_Selected() { anglebutton[1]->setOn(ttrue); }
void QtCalculator::Gra_Selected() { anglebutton[2]->setOn(ttrue); }

/*
 * react to buttons press/unpress events:
 */
void QtCalculator::Hex_Toggled(TEvent *E, void *Arg) {
  if (isDown(E)) SetHex();
}
void QtCalculator::Dec_Toggled(TEvent *E, void *Arg) {
  if (isDown(E)) SetDec();
}
void QtCalculator::Oct_Toggled(TEvent *E, void *Arg) {
  if (isDown(E)) SetOct();
}
void QtCalculator::Bin_Toggled(TEvent *E, void *Arg) {
  if (isDown(E)) SetBin();
}

void QtCalculator::Deg_Toggled(TEvent *E, void *Arg) {
  if (isDown(E)) SetDeg();
}
void QtCalculator::Rad_Toggled(TEvent *E, void *Arg) {
  if (isDown(E)) SetRad();
}
void QtCalculator::Gra_Toggled(TEvent *E, void *Arg) {
  if (isDown(E)) SetGra();
}

void QtCalculator::configclicked() {
#if 0
  QTabDialog *tabdialog;
  tabdialog = new QTabDialog(0,"tabdialog",ttrue);

  tabdialog->setCaption( i18n("KCalc Configuraton") );
  tabdialog->resize( 350, 350 );
  tabdialog->setCancelButton( i18n("Cancel") );

  QWidget *about = new QWidget(tabdialog,"about");

  QGroupBox *box = new QGroupBox(about,"box");
  TGadget  *label = new TGadget(box,"label");
  TGadget  *label2 = new TGadget(box,"label2");
  box->setXY(10,10,320,260);

  box->setTitle(i18n("About"));


  label->setXY(140,30,160,170);
  label2->setXY(20,150,280,100);

  QString labelstring;
  labelstring.sprintf(i18n("KCalc %s\n"
			   "Bernd Johannes Wuebben\n"
			   "wuebben@math.cornell.edu\n"
			   "wuebben@kde.org\n"
			   "Copyright (C) 1996-98\n"
			   "\n\n"), KCALCVERSION);

  QString labelstring2 =
#ifdef HAVE_LONG_DOUBLE
		i18n( "Base type: long double\n");
#else
		i18n( "Due to broken glibc's everywhere, "\
		      "I had to reduce KCalc's precision from 'long double' "\
		      "to 'double'. "\
		      "Owners of systems with a working libc "\
		      "should recompile KCalc with 'long double' precision "\
		      "enabled. See the README for details.");
#endif

  label->setAlignment(AlignLeft|WordBreak|ExpandTabs);
  label->setText(labelstring.data());

  label2->setAlignment(AlignLeft|WordBreak|ExpandTabs);
  label2->setText(labelstring2.data());

  QString pixdir = mykapp->kde_datadir() + "/kcalc/pics/";


  QPixmap pm((pixdir + "kcalclogo.xpm").data());
  TGadget *logo = new TGadget(box);
  logo->setPixmap(pm);
  logo->setXY(30, 20, pm.width(), pm.height());


  DefStruct newdefstruct;
  newdefstruct.forecolor  = kcalcdefaults.forecolor;
  newdefstruct.backcolor  = kcalcdefaults.backcolor;
  newdefstruct.precision  = kcalcdefaults.precision;
  newdefstruct.fixedprecision  = kcalcdefaults.fixedprecision;
  newdefstruct.fixed  = kcalcdefaults.fixed;
  newdefstruct.style  = kcalcdefaults.style;
  newdefstruct.beep  = kcalcdefaults.beep;

  ConfigDlg *configdlg;
  configdlg = new ConfigDlg(tabdialog,"configdlg",mykapp,&newdefstruct);

  tabdialog->addTab(configdlg,i18n("Defaults"));
  tabdialog->addTab(about,i18n("About"));


  if(tabdialog->exec() == QDialog::Accepted){


    kcalcdefaults.forecolor  = newdefstruct.forecolor;
    kcalcdefaults.backcolor  = newdefstruct.backcolor;
    kcalcdefaults.precision  = newdefstruct.precision;
    kcalcdefaults.fixedprecision  = newdefstruct.fixedprecision;
    kcalcdefaults.fixed  = newdefstruct.fixed;
    kcalcdefaults.style  = newdefstruct.style;
    kcalcdefaults.beep  = newdefstruct.beep;

    set_colors();
    set_precision();
    set_style();
  }
#endif /* 0 */
}

void QtCalculator::set_style() {
  switch (kcalcdefaults.style) {
    case 0: {
      pbhyp->setText("Hyp");
      pbSin->setText("Sin");
      pbCos->setText("Cos");
      pbTan->setText("Tan");
      pblog->setText("Log");
      pbln->setText("Ln ");
      break;
    }
    case 1: {
      pbhyp->setText(" N ");
      pbSin->setText("Mea");
      pbCos->setText("Std");
      pbTan->setText("Med");
      pblog->setText("Dat");
      pbln->setText("CSt");
      break;
    }

    default:
      break;
  }
}

void QtCalculator::readSettings() {
#ifdef HAVE_LONG_DOUBLE
  kcalcdefaults.precision = 14;
#else
  kcalcdefaults.precision = 10;
#endif
  kcalcdefaults.fixedprecision = 2;
  kcalcdefaults.fixed = 0;

#if 0
  QString str;

  KConfig *config = mykapp->getConfig();

  config->setGroup("Colors");
  TColor tmpC(189, 255, 222);
  TColor blackC(0,0,0);

  kcalcdefaults.forecolor = config->readColorEntry("ForeColor",&blackC);
  kcalcdefaults.backcolor = config->readColorEntry("BackColor",&tmpC);

  config->setGroup("Precision");

#ifdef HAVE_LONG_DOUBLE
  kcalcdefaults.precision =  config->readNumEntry("precision",(int)14);
#else
  kcalcdefaults.precision =  config->readNumEntry("precision",(int)10);
#endif

  kcalcdefaults.fixedprecision =  config->readNumEntry("fixedprecision",(int)2);
  kcalcdefaults.fixed = (bool) config->readNumEntry("fixed",(int)0);

  config->setGroup("General");
  kcalcdefaults.style          = config->readNumEntry("style",(int)0);
  kcalcdefaults.beep          = config->readNumEntry("beep",(int)1);
#endif /* 0 */
}

void QtCalculator::writeSettings() {
#if 0
  KConfig *config = mykapp->getConfig();

  config->setGroup( "Font" );

  config->setGroup("Colors");
  config->writeEntry("ForeColor",kcalcdefaults.forecolor);
  config->writeEntry("BackColor",kcalcdefaults.backcolor);

  config->setGroup("Precision");
  config->writeEntry("precision",  kcalcdefaults.precision);
  config->writeEntry("fixedprecision",  kcalcdefaults.fixedprecision);
  config->writeEntry("fixed",  (int)kcalcdefaults.fixed);

  config->setGroup("General");
  config->writeEntry("style",(int)kcalcdefaults.style);
  config->writeEntry("beep",(int)kcalcdefaults.beep);
  config->sync();
#endif /* 0 */
}

/* we finally managed to receive the selection contents */
void QtCalculator::notified_selection(TEvent *E, void *arg) {
  TClipboard *cb = E->clipboard();

  CALCAMNT result;
  const char *text = cb->data();

  if (cb->magic() != TW_SEL_UTF8MAGIC) {
    /* FIXME: handle non-utf8 data */
    return;
  } else {
    result = (CALCAMNT)strtod(text ? text : "0", 0);
  }
  //    printf("%Lg\n",result);
  last_input = PASTE;
  DISPLAY_AMOUNT = result;
  UpdateDisplay();
}

void QtCalculator::clear_status_label() { statusERRORLabel->setText(""); }

void QtCalculator::setStatusLabel(const char *string) {
  statusERRORLabel->setText(string);
}

void QtCalculator::closeEvent(TCloseEvent *E) { quitCalc(); }

void QtCalculator::quitCalc() {
  writeSettings();
  quit();
}

void QtCalculator::set_colors() {
#if 0
  QPalette mypalette = (calc_display->palette()).copy();

  TColorGroup cgrp = mypalette.normal();
  TColorGroup ncgrp(kcalcdefaults.forecolor,
		    cgrp.background(),
		    cgrp.light(),
		    cgrp.dark(),
		    cgrp.mid(),
		    kcalcdefaults.forecolor,
		    kcalcdefaults.backcolor);

  mypalette.setNormal(ncgrp);
  mypalette.setDisabled(ncgrp);
  mypalette.setActive(ncgrp);

  calc_display->setPalette(mypalette);
  calc_display->setBackgroundColor(kcalcdefaults.backcolor);
#endif /* 0 */
}

void QtCalculator::set_precision() { UpdateDisplay(); }

void QtCalculator::temp_stack_next() {
  CALCAMNT *number;

  if (temp_stack.current() == temp_stack.getLast()) {
    QApplication::beep();
    return;
  }

  number = temp_stack.next();

  if (number == NULL) {
    QApplication::beep();
    return;
  } else {
    //    printf("Number: %Lg\n",*number);
    last_input = RECALL;
    DISPLAY_AMOUNT = *number;
    UpdateDisplay();
  }
}

void QtCalculator::temp_stack_prev() {
  CALCAMNT *number;

  if (temp_stack.current() == temp_stack.getFirst()) {
    QApplication::beep();
    return;
  }

  number = temp_stack.prev();

  if (number == NULL) {
    QApplication::beep();
    return;
  } else {
    //    printf("Number: %Lg\n",*number);
    last_input = RECALL;
    DISPLAY_AMOUNT = *number;
    UpdateDisplay();
  }
}

////////////////////////////////////////////////////////////////
// Include the meta-object code for classes in this file
//

/////////////////////////////////////////////////////////////////
// Create and display our WidgetView.
//

int main(int argc, char **argv) {
  const char *argv0 = *argv;
  const char *fatal = NULL;

  if (argc > 1) {
    argv++;
    for (int i = 1; i <= argc; i++) {
      if (strcmp(*argv, "-v") == 0 || strcmp(*argv, "-h") == 0) {
        printf(
            "TwKalc %s\n"
            "Copyright 1997 Bernd Johannes Wuebben"
            " <wuebben@kde.org>\n"
            "Copyright 2001 Massimiliano Ghilardi "
            " <max@linuz.sns.it>\n",
            KCALCVERSION);
        exit(1);
      }
      argv++;
    }
  }

  if ((dpy = new TW()) && dpy->open() && (new TMsgPort(dpy, "twkalc")) &&
      (Menu = new TMenu())) {
    Menu->commonItem();

    QtCalculator *QtCalc = new QtCalculator("twkalc");
    QtCalc->map(dpy->firstScreen());
    dpy->exec();
  }
  if (dpy->error()) dpy->dumpError(argv0);
  return 1;
}
