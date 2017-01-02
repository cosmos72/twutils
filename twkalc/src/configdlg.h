
/*

 $Id: configdlg.h,v 1.2 1997/09/21 04:41:09 wuebben Exp $


 KCalc

 Copyright (C) Bernd Johannes Wuebben
               wuebben@math.cornell.edu
	       wuebben@kde.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 */


#ifndef _CONFIG_DLG_H_
#define _CONFIG_DLG_H_

#include "kcalc.h"



class ConfigDlg : public TWindow {

public:

  ConfigDlg(TWidget *parent=0, const char *name=0);

  ~ConfigDlg() {}

  DefStruct *defst ;


//private slots:

  void okButton();
  void cancelbutton();
  void set_fore_color();
  void set_background_color();
  void help();

//signals:
  void color_change();

public:
  bool colors_changed;
  
private:


  TGroup *box;
  
  TGadget *ok;
  TGadget *cancel;

  TGadget *label1;
  TWidget *qframe1;
  TGadget *button1;

  TGadget *label2;
  TWidget *qframe2;
  TGadget *button2;

  TGadget *label3;
  TWidget *qframe3;
  TGadget *button3;

  TGadget *label4;
  TWidget *qframe4;
  TGadget *button4;
//  KApplication* mykapp; // never use kapp;

  TGadget *label5;

  TGadget *label6;


  TGadget *label7;

  TGadget *label8;


  TGroup *gbox;
  TGadget *cb;  
  TGadget *cb2;
//  KNumericSpinBox* precspin;
//  KNumericSpinBox* precspin2;  
  TGadget *mybox;
  TGadget *frame3d;

  TGroup *stylegroup;
  TGadget *stylelabel;
  TGadget *trigstyle;
  TGadget *statstyle;

};
#endif

