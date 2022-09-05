/*
    $Id: kcalc.h,v 1.14 1999/01/12 09:20:32 kulow Exp $

    KCalc, a scientific calculator for the X window system using the
    Qt widget libraries, available at no cost at http://www.troll.no

    Copyright (C) 1996 Bernd Johannes Wuebben
                       wuebben@math.cornell.edu

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

#ifndef KCALC_H
#define KCALC_H

#include <Tw/Tw++.h>
#include <Tw/Twkeys.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

extern TW* dpy;

#include "TList.h"

typedef void* GCI;

#include "stats.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern TW* dpy;

// IMPORTANT this has to come after config.h
#include "kcalctype.h"

#define STACK_SIZE 100
#define TEMP_STACK_SIZE \
  1000  // the number of numbers kept in the temp stack
        // which are accessible with the up and down arrow
        // key

#define PRECEDENCE_INCR 20

#define FUNC_NULL 0
#define FUNC_OR 1
#define FUNC_XOR 2
#define FUNC_AND 3
#define FUNC_LSH 4
#define FUNC_RSH 5
#define FUNC_ADD 6
#define FUNC_SUBTRACT 7
#define FUNC_MULTIPLY 8
#define FUNC_DIVIDE 9
#define FUNC_MOD 10
#define FUNC_POWER 11
#define FUNC_PWR_ROOT 12
#define FUNC_INTDIV 13

#define DEC_SIZE 19
#define BOH_SIZE 16
#define DSP_SIZE 50  // 25

#define DEG2RAD(x) (((2L * pi) / 360L) * x)
#define GRA2RAD(x) ((pi / 200L) * x)
#define RAD2DEG(x) ((360L / (2L * pi)) * x)
#define RAD2GRA(x) ((200L / pi) * x)
#define POS_ZERO 1e-19L  /* What we consider zero is   */
#define NEG_ZERO -1e-19L /* anything between these two */

typedef CALCAMNT (*Arith)(CALCAMNT, CALCAMNT);
typedef CALCAMNT (*Prcnt)(CALCAMNT, CALCAMNT, CALCAMNT);
typedef CALCAMNT (*Trig)(CALCAMNT);

typedef enum _last_input_type {
  DIGIT = 1,
  OPERATION = 2,
  RECALL = 3,
  PASTE = 4
} last_input_type;

typedef enum _num_base {
  NB_BINARY = 2,
  NB_OCTAL = 8,
  NB_DECIMAL = 10,
  NB_HEX = 16
} num_base;

typedef enum _angle_type {
  ANG_DEGREE = 0,
  ANG_RADIAN = 1,
  ANG_GRADIENT = 2
} angle_type;

typedef enum _item_type { ITEM_FUNCTION, ITEM_AMOUNT } item_type;

typedef struct _func_data {
  int item_function;
  int item_precedence;
} func_data;

typedef union _item_data {  /* The item data	 */
  CALCAMNT item_amount;     /*	an amount	 */
  func_data item_func_data; /*	or a function	 */
} item_data;                /* called item_data      */

typedef struct _item_contents { /* The item contents	 */
  item_type s_item_type;        /* 	a type flag      */
  item_data s_item_data;        /*      and data	 */
} item_contents;

typedef struct stack_item* stack_ptr;

typedef struct stack_item {
  /* Contents of an item on the input stack */

  stack_ptr prior_item;     /* Pointer to prior item */
  stack_ptr prior_type;     /* Pointer to prior type */
  item_contents item_value; /* The value of the item */

} stack_item; /* all called stack_item */

CALCAMNT ExecOr(CALCAMNT left_op, CALCAMNT right_op);
CALCAMNT ExecXor(CALCAMNT left_op, CALCAMNT right_op);
CALCAMNT ExecAnd(CALCAMNT left_op, CALCAMNT right_op);
CALCAMNT ExecLsh(CALCAMNT left_op, CALCAMNT right_op);
CALCAMNT ExecRsh(CALCAMNT left_op, CALCAMNT right_op);
CALCAMNT ExecAdd(CALCAMNT left_op, CALCAMNT right_op);
CALCAMNT ExecSubtract(CALCAMNT left_op, CALCAMNT right_op);
CALCAMNT ExecMultiply(CALCAMNT left_op, CALCAMNT right_op);
CALCAMNT ExecDivide(CALCAMNT left_op, CALCAMNT right_op);
CALCAMNT ExecMod(CALCAMNT left_op, CALCAMNT right_op);
CALCAMNT ExecPower(CALCAMNT left_op, CALCAMNT right_op);
CALCAMNT ExecPwrRoot(CALCAMNT left_op, CALCAMNT right_op);
CALCAMNT ExecIntDiv(CALCAMNT left_op, CALCAMNT right_op);

CALCAMNT ExecAddSubP(CALCAMNT left_op, CALCAMNT right_op, CALCAMNT result);
CALCAMNT ExecMultiplyP(CALCAMNT left_op, CALCAMNT right_op, CALCAMNT result);
CALCAMNT ExecDivideP(CALCAMNT left_op, CALCAMNT right_op, CALCAMNT result);
CALCAMNT ExecPowerP(CALCAMNT left_op, CALCAMNT right_op, CALCAMNT result);
CALCAMNT ExecPwrRootP(CALCAMNT left_op, CALCAMNT right_op, CALCAMNT result);

int UpdateStack(int run_precedence);
CALCAMNT ExecFunction(CALCAMNT left_op, int function, CALCAMNT right_op);
int cvb(char* out_str, long amount, int max_out);

void PrintStack(void);
void InitStack(void);
void PushStack(item_contents* add_item);
item_contents* PopStack(void);
item_contents* TopOfStack(void);
item_contents* TopTypeStack(item_type rqstd_type);

#define DISPLAY_AMOUNT display_data.s_item_data.item_amount

typedef struct _DefStruct {
  TColor forecolor;
  TColor backcolor;
  int precision;
  int fixedprecision;
  int style;
  bool fixed;
  bool beep;
} DefStruct;

class QtCalculator : public TWindow {
 public:
  QtCalculator(const char* name = 0);

  void defaultListener(TMsg* M, void* Arg);
  void keyPressEvent(TKeyEvent* e);
  void keyReleaseEvent(TKeyEvent* e);
  void closeEvent(TCloseEvent* e);
  void writeSettings();
  void readSettings();
  void set_precision();
  void set_display_font();
  void set_style();
  void temp_stack_next();
  void temp_stack_prev();
  void ComputeMean();
  void ComputeSin();
  void ComputeStd();
  void ComputeCos();
  void ComputeMedean();
  void ComputeTan();

  // public slots:

  void helpclicked();
  void set_colors();
  /* user clicked on display. let's declare we own the selection */
  static void display_clicked() { dpy->exportSelection(); }
  /* user clicked to paste selection. let's ask to who owns it. */
  /* twin server BUG: we get this only if user middle-clicks outside
   * gadgets/buttons */
  static void user_wants_to_paste() { dpy->requestSelection(); }
  /* some client wants our selection. let's give it to him */
  static void someclient_wants_selection(TEvent* E, void*) {
    extern char display_str[];
    static char buf[TW_MAX_MIMELEN];

    TSelectionRequestEvent* rq = E->selectionRequestEvent();
    dpy->notifySelection(rq->requestor(), rq->reqprivate(), TW_SEL_UTF8MAGIC,
                         buf, strlen(display_str), display_str);
  }
  void notified_selection(TEvent* E, void* arg);
  void quitCalc();
  void clear_buttons();
  void clear_status_label();
  void setStatusLabel(const char*);
  void EnterDigit(int data);
  void EnterDecimal();
  void EnterStackFunction(int data);
  void EnterNegate();
  void EnterOpenParen();
  void EnterCloseParen();
  void EnterRecip();
  void EnterInt();
  void EnterFactorial();
  void EnterSquare();
  void EnterNotCmp();
  void EnterHyp();
  void EnterPercent();
  void EnterLogr();
  void EnterLogn();
  void SetDeg();
  void SetGra();
  void SetRad();
  void SetHex();
  void SetOct();
  void SetBin();
  void SetDec();
  void Deg_Selected();
  void Rad_Selected();
  void Gra_Selected();
  void Hex_Selected();
  void Dec_Selected();
  void Oct_Selected();
  void Bin_Selected();
  void Deg_Toggled(TEvent* E, void* Arg);
  void Rad_Toggled(TEvent* E, void* Arg);
  void Gra_Toggled(TEvent* E, void* Arg);
  void Hex_Toggled(TEvent* E, void* Arg);
  void Dec_Toggled(TEvent* E, void* Arg);
  void Oct_Toggled(TEvent* E, void* Arg);
  void Bin_Toggled(TEvent* E, void* Arg);
  void SetInverse();
  void EnterEqual();
  void Clear();
  void ClearAll();
  void RefreshCalculator(void);
  void InitializeCalculator(void);
  void UpdateDisplay();
  void ExecSin();
  void ExecCos();
  void ExecTan();
  void button0();
  void button1();
  void button2();
  void button3();
  void button4();
  void button5();
  void button6();
  void button7();
  void button8();
  void button9();
  void buttonA();
  void buttonB();
  void buttonC();
  void buttonD();
  void buttonE();
  void buttonF();
  void Or();
  void And();
  void Shift();
  void Plus();
  void Minus();
  void Multiply();
  void Divide();
  void Mod();
  void Power();
  void EE();
  void MR();
  void Mplusminus();
  void MC();
  void quit();
  void configclicked();

 public:
  DefStruct kcalcdefaults;

 private:
  TGadget* statusINVLabel;
  TGadget* statusHYPLabel;
  TGadget* statusERRORLabel;
  TGadget* calc_display;
  TGadget* anglebutton[3];
  TGadget* basebutton[4];
  TGadget* pbhyp;
  TGadget* pbEE;
  TGadget* pbinv;
  TGadget* pbMR;
  TGadget* pbA;
  TGadget* pbSin;
  TGadget* pbplusminus;
  TGadget* pbMplusminus;
  TGadget* pbB;
  TGadget* pbCos;
  TGadget* pbreci;
  TGadget* pbC;
  TGadget* pbTan;
  TGadget* pbfactorial;
  TGadget* pbD;
  TGadget* pblog;
  TGadget* pbsquare;
  TGadget* pbE;
  TGadget* pbln;
  TGadget* pbpower;
  TGadget* pbF;
  TGadget* pbMC;
  TGadget* pbClear;
  TGadget* pbAC;
  TGadget* pb7;
  TGadget* pb8;
  TGadget* pb9;
  TGadget* pbparenopen;
  TGadget* pbparenclose;
  TGadget* pband;
  TGadget* pb4;
  TGadget* pb5;
  TGadget* pb6;
  TGadget* pbX;
  TGadget* pbdivision;
  TGadget* pbor;
  TGadget* pb1;
  TGadget* pb2;
  TGadget* pb3;
  TGadget* pbplus;
  TGadget* pbminus;
  TGadget* pbshift;
  TGadget* pbperiod;
  TGadget* pb0;
  TGadget* pbequal;
  TGadget* pbpercent;
  TGadget* pbnegate;
  TGadget* pbmod;

  bool key_pressed;
  KStats stats;
};

class QApplication {
 public:
  static inline void beep() { TW::beep(); }
};

#endif  // QTCLAC_H
