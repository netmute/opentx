/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "rawsource.h"

#include "eeprominterface.h"
#include "radiodata.h"
#include "radiodataconversionstate.h"

#include <float.h>

/*
 * RawSourceRange
 */

float RawSourceRange::getValue(int value)
{
  if (IS_ARM(getCurrentBoard()))
    return float(value) * step;
  else
    return min + float(value) * step;
}


/*
 * RawSource
 */

RawSourceRange RawSource::getRange(const ModelData * model, const GeneralSettings & settings, unsigned int flags) const
{
  RawSourceRange result;

  Firmware * firmware = Firmware::getCurrentVariant();
  int board = firmware->getBoard();

  switch (type) {
    case SOURCE_TYPE_TELEMETRY:
      if (IS_ARM(board)) {
        div_t qr = div(index, 3);
        const SensorData & sensor = model->sensorData[qr.quot];
        if (sensor.prec == 2)
          result.step = 0.01;
        else if (sensor.prec == 1)
          result.step = 0.1;
        else
          result.step = 1;
        result.min = -30000 * result.step;
        result.max = +30000 * result.step;
        result.decimals = sensor.prec;
        result.unit = sensor.unitString();
      }
      else {
        result.offset = -DBL_MAX;

        switch (index) {
          case TELEMETRY_SOURCE_TX_BATT:
            result.step = 0.1;
            result.decimals = 1;
            result.max = 25.5;
            result.unit = QObject::tr("V");
            break;
          case TELEMETRY_SOURCE_TX_TIME:
            result.step = 60;
            result.max = 24 * 60 * result.step - 60;  // 23:59:00 with 1-minute resolution
            result.unit = QObject::tr("s");
            break;
          case TELEMETRY_SOURCE_TIMER1:
          case TELEMETRY_SOURCE_TIMER2:
          case TELEMETRY_SOURCE_TIMER3:
            result.step = 5;
            result.max = 255 * result.step;
            result.unit = QObject::tr("s");
            break;
          case TELEMETRY_SOURCE_RSSI_TX:
          case TELEMETRY_SOURCE_RSSI_RX:
            result.max = 100;
            result.offset = 128;
            break;
          case TELEMETRY_SOURCE_A1_MIN:
          case TELEMETRY_SOURCE_A2_MIN:
          case TELEMETRY_SOURCE_A3_MIN:
          case TELEMETRY_SOURCE_A4_MIN:
            if (model) result = model->frsky.channels[index-TELEMETRY_SOURCE_A1_MIN].getRange();
            break;
          case TELEMETRY_SOURCE_A1:
          case TELEMETRY_SOURCE_A2:
          case TELEMETRY_SOURCE_A3:
          case TELEMETRY_SOURCE_A4:
            if (model) result = model->frsky.channels[index-TELEMETRY_SOURCE_A1].getRange();
            break;
          case TELEMETRY_SOURCE_ALT:
          case TELEMETRY_SOURCE_ALT_MIN:
          case TELEMETRY_SOURCE_ALT_MAX:
          case TELEMETRY_SOURCE_GPS_ALT:
            result.step = 8;
            result.min = -500;
            result.max = 1540;
            if (firmware->getCapability(Imperial) || settings.imperial) {
              result.step = (result.step * 105) / 32;
              result.min = (result.min * 105) / 32;
              result.max = (result.max * 105) / 32;
              result.unit = QObject::tr("ft");
            }
            else {
              result.unit = QObject::tr("m");
            }
            break;
          case TELEMETRY_SOURCE_T1:
          case TELEMETRY_SOURCE_T1_MAX:
          case TELEMETRY_SOURCE_T2:
          case TELEMETRY_SOURCE_T2_MAX:
            result.min = -30;
            result.max = 225;
            result.unit = QObject::trUtf8("°C");
            break;
          case TELEMETRY_SOURCE_HDG:
            result.step = 2;
            result.max = 360;
            result.offset = 256;
            result.unit = QObject::trUtf8("°");
            break;
          case TELEMETRY_SOURCE_RPM:
          case TELEMETRY_SOURCE_RPM_MAX:
            result.step = 50;
            result.max = 12750;
            break;
          case TELEMETRY_SOURCE_FUEL:
            result.max = 100;
            result.unit = QObject::tr("%");
            break;
          case TELEMETRY_SOURCE_ASPEED:
          case TELEMETRY_SOURCE_ASPEED_MAX:
            result.decimals = 1;
            result.step = 2.0;
            result.max = (2*255);
            if (firmware->getCapability(Imperial) || settings.imperial) {
              result.step *= 1.150779;
              result.max *= 1.150779;
              result.unit = QObject::tr("mph");
            }
            else {
              result.step *= 1.852;
              result.max *= 1.852;
              result.unit = QObject::tr("km/h");
            }
            break;
          case TELEMETRY_SOURCE_SPEED:
          case TELEMETRY_SOURCE_SPEED_MAX:
            result.step = 2;
            result.max = (2*255);
            if (firmware->getCapability(Imperial) || settings.imperial) {
              result.step *= 1.150779;
              result.max *= 1.150779;
              result.unit = QObject::tr("mph");
            }
            else {
              result.step *= 1.852;
              result.max *= 1.852;
              result.unit = QObject::tr("km/h");
            }
            break;
          case TELEMETRY_SOURCE_VERTICAL_SPEED:
            result.step = 0.1;
            result.min = -12.5;
            result.max = 13.0;
            result.decimals = 1;
            result.unit = QObject::tr("m/s");
            break;
          case TELEMETRY_SOURCE_DTE:
            result.max = 30000;
            break;
          case TELEMETRY_SOURCE_DIST:
          case TELEMETRY_SOURCE_DIST_MAX:
            result.step = 8;
            result.max = 2040;
            result.unit = QObject::tr("m");
            break;
          case TELEMETRY_SOURCE_CELL:
          case TELEMETRY_SOURCE_CELL_MIN:
            result.step = 0.02;
            result.max = 5.1;
            result.decimals = 2;
            result.unit = QObject::tr("V");
            break;
          case TELEMETRY_SOURCE_CELLS_SUM:
          case TELEMETRY_SOURCE_CELLS_MIN:
          case TELEMETRY_SOURCE_VFAS:
          case TELEMETRY_SOURCE_VFAS_MIN:
            result.step = 0.1;
            result.max = 25.5;
            result.decimals = 1;
            result.unit = QObject::tr("V");
            break;
          case TELEMETRY_SOURCE_CURRENT:
          case TELEMETRY_SOURCE_CURRENT_MAX:
            result.step = 0.5;
            result.max = 127.5;
            result.decimals = 1;
            result.unit = QObject::tr("A");
            break;
          case TELEMETRY_SOURCE_CONSUMPTION:
            result.step = 100;
            result.max =  25500;
            result.unit = QObject::tr("mAh");
            break;
          case TELEMETRY_SOURCE_POWER:
          case TELEMETRY_SOURCE_POWER_MAX:
            result.step = 5;
            result.max = 1275;
            result.unit = QObject::tr("W");
            break;
          case TELEMETRY_SOURCE_ACCX:
          case TELEMETRY_SOURCE_ACCY:
          case TELEMETRY_SOURCE_ACCZ:
            result.step = 0.01;
            result.decimals = 2;
            result.max = 2.55;
            result.min = 0;
            result.unit = QObject::tr("g");
            break;
          default:
            result.max = 125;
            break;
        }

        if (result.offset == -DBL_MAX) {
          result.offset = result.max - (127*result.step);
        }

        if (flags & (RANGE_DELTA_FUNCTION | RANGE_ABS_FUNCTION)) {
          result.offset = 0;
          result.min = result.step * -127;
          result.max = result.step * 127;
        }
      }
      break;

    case SOURCE_TYPE_LUA_OUTPUT:
      result.max = 30000;
      result.min = -result.max;
      break;

    case SOURCE_TYPE_TRIM:
      result.max = (model && model->extendedTrims ? firmware->getCapability(ExtendedTrimsRange) : firmware->getCapability(TrimsRange));
      result.min = -result.max;
      break;

    case SOURCE_TYPE_GVAR: {
      GVarData gv = model->gvarData[index];
      result.step = gv.multiplierGet();
      result.decimals = gv.prec;
      result.max = gv.getMaxPrec();
      result.min = gv.getMinPrec();
      result.unit = gv.unitToString();
      break;
    }

    case SOURCE_TYPE_SPECIAL:
      if (index == 0)  {  //Batt
        result.step = 0.1;
        result.decimals = 1;
        result.max = 25.5;
        result.unit = QObject::tr("V");
      }
      else if (index == 1) {   //Time
        result.step = 60;
        result.max = 24 * 60 * result.step - 60;  // 23:59:00 with 1-minute resolution
        result.unit = QObject::tr("s");
      }
      else {      // Timers 1 - 3
        result.step = 1;
        result.max = 9 * 60 * 60 - 1;  // 8:59:59 (to match firmware)
        result.min = -result.max;
        result.unit = QObject::tr("s");
      }
      break;

    case SOURCE_TYPE_CH:
      result.max = model->getChannelsMax(false);
      result.min = -result.max;
      break;

    default:
      result.max = 100;
      result.min = -result.max;
      break;
  }

  if (flags & RANGE_ABS_FUNCTION) {
    result.min = 0;
  }

  return result;
}

QString RawSource::toString(const ModelData * model, const GeneralSettings * const generalSettings, Board::Type board) const
{
  if (board == Board::BOARD_UNKNOWN) {
    board = getCurrentBoard();
  }

  static const QString trims[] = {
    QObject::tr("TrmR"), QObject::tr("TrmE"), QObject::tr("TrmT"), QObject::tr("TrmA"), QObject::tr("Trm5"), QObject::tr("Trm6")
  };

  static const QString special[] = {
    QObject::tr("Batt"), QObject::tr("Time"), QObject::tr("Timer1"), QObject::tr("Timer2"), QObject::tr("Timer3"),
  };

  static const QString telemetry[] = {
    QObject::tr("Batt"), QObject::tr("Time"), QObject::tr("Timer1"), QObject::tr("Timer2"), QObject::tr("Timer3"),
    QObject::tr("SWR"), QObject::tr("RSSI Tx"), QObject::tr("RSSI Rx"),
    QObject::tr("A1"), QObject::tr("A2"), QObject::tr("A3"), QObject::tr("A4"),
    QObject::tr("Alt"), QObject::tr("Rpm"), QObject::tr("Fuel"), QObject::tr("T1"), QObject::tr("T2"),
    QObject::tr("Speed"), QObject::tr("Dist"), QObject::tr("GPS Alt"),
    QObject::tr("Cell"), QObject::tr("Cells"), QObject::tr("Vfas"), QObject::tr("Curr"), QObject::tr("Cnsp"), QObject::tr("Powr"),
    QObject::tr("AccX"), QObject::tr("AccY"), QObject::tr("AccZ"),
    QObject::tr("Hdg "), QObject::tr("VSpd"), QObject::tr("AirSpeed"), QObject::tr("dTE"),
    QObject::tr("A1-"),  QObject::tr("A2-"), QObject::tr("A3-"),  QObject::tr("A4-"),
    QObject::tr("Alt-"), QObject::tr("Alt+"), QObject::tr("Rpm+"), QObject::tr("T1+"), QObject::tr("T2+"), QObject::tr("Speed+"), QObject::tr("Dist+"), QObject::tr("AirSpeed+"),
    QObject::tr("Cell-"), QObject::tr("Cells-"), QObject::tr("Vfas-"), QObject::tr("Curr+"), QObject::tr("Powr+"),
    QObject::tr("ACC"), QObject::tr("GPS Time"),
  };

  static const QString rotary[]  = { QObject::tr("REa"), QObject::tr("REb") };

  if (index<0) {
    return QObject::tr("???");
  }

  QString result;
  int genAryIdx = 0;
  switch (type) {
    case SOURCE_TYPE_NONE:
      return QObject::tr("----");

    case SOURCE_TYPE_VIRTUAL_INPUT:
    {
      const char * name = NULL;
      if (model)
        name = model->inputNames[index];
      return RadioData::getElementName(QCoreApplication::translate("Input", "I"), index + 1, name);
    }

    case SOURCE_TYPE_LUA_OUTPUT:
      return QObject::tr("LUA%1%2").arg(index/16+1).arg(QChar('a'+index%16));

    case SOURCE_TYPE_STICK:
      if (generalSettings) {
        if (isPot(&genAryIdx))
          result = QString(generalSettings->potName[genAryIdx]);
        else if (isSlider(&genAryIdx))
          result = QString(generalSettings->sliderName[genAryIdx]);
        else if (isStick(&genAryIdx))
          result = QString(generalSettings->stickName[genAryIdx]);
      }
      if (result.isEmpty())
        result = Boards::getAnalogInputName(board, index);
      return result;

    case SOURCE_TYPE_TRIM:
      return CHECK_IN_ARRAY(trims, index);
    case SOURCE_TYPE_ROTARY_ENCODER:
      return CHECK_IN_ARRAY(rotary, index);
    case SOURCE_TYPE_MAX:
      return QObject::tr("MAX");

    case SOURCE_TYPE_SWITCH:
      if (generalSettings)
        result = QString(generalSettings->switchName[index]);
      if (result.isEmpty())
        result = Boards::getSwitchInfo(board, index).name;
      return result;

    case SOURCE_TYPE_CUSTOM_SWITCH:
      return RawSwitch(SWITCH_TYPE_VIRTUAL, index+1).toString();

    case SOURCE_TYPE_CYC:
      return QObject::tr("CYC%1").arg(index+1);

    case SOURCE_TYPE_PPM:
      return RadioData::getElementName(QCoreApplication::translate("Trainer", "TR"), index + 1);

    case SOURCE_TYPE_CH:
    {
      const char * name = NULL;
      if (getCurrentFirmware()->getCapability(ChannelsName) && model)
        name = model->limitData[index].name;
      return RadioData::getElementName(QCoreApplication::translate("Channel", "CH"), index + 1, name);
    }

    case SOURCE_TYPE_SPECIAL:
      return CHECK_IN_ARRAY(special, index);

    case SOURCE_TYPE_TELEMETRY:
      if (IS_ARM(board)) {
        div_t qr = div(index, 3);
        result = RadioData::getElementName(QCoreApplication::translate("Telemetry", "TELE"), qr.quot+1, model ? model->sensorData[qr.quot].label : NULL);
        if (qr.rem)
          result += (qr.rem == 1 ? "-" : "+");
        return result;
      }
      else {
        return CHECK_IN_ARRAY(telemetry, index);
      }

    case SOURCE_TYPE_GVAR:
    {
      const char * name = NULL;
      if (getCurrentFirmware()->getCapability(GvarsName) && model)
        name = model->gvarData[index].name;
      return RadioData::getElementName(QCoreApplication::translate("Global Variable", "GV"), index + 1, name);
    }

    default:
      return QObject::tr("???");
  }
}

bool RawSource::isStick(int * stickIndex, Board::Type board) const
{
  if (board == Board::BOARD_UNKNOWN)
    board = getCurrentBoard();

  if (type == SOURCE_TYPE_STICK && index < Boards::getCapability(board, Board::Sticks)) {
    if (stickIndex)
      *stickIndex = index;
    return true;
  }
  return false;
}

bool RawSource::isPot(int * potsIndex, Board::Type board) const
{
  if (board == Board::BOARD_UNKNOWN)
    board = getCurrentBoard();

  Boards b(board);
  if (type == SOURCE_TYPE_STICK &&
          index >= b.getCapability(Board::Sticks) &&
          index < b.getCapability(Board::Sticks) + b.getCapability(Board::Pots)) {
    if (potsIndex)
      *potsIndex = index - b.getCapability(Board::Sticks);
    return true;
  }
  return false;
}

bool RawSource::isSlider(int * sliderIndex, Board::Type board) const
{
  if (board == Board::BOARD_UNKNOWN)
    board = getCurrentBoard();

  Boards b(board);
  if (type == SOURCE_TYPE_STICK &&
          index >= b.getCapability(Board::Sticks) + b.getCapability(Board::Pots) &&
          index < b.getCapability(Board::Sticks) + b.getCapability(Board::Pots) + b.getCapability(Board::Sliders)) {
    if (sliderIndex)
      *sliderIndex = index - b.getCapability(Board::Sticks) - b.getCapability(Board::Pots);
    return true;
  }
  return false;
}

bool RawSource::isTimeBased(Board::Type board) const
{
  if (board == Board::BOARD_UNKNOWN)
    board = getCurrentBoard();

  if (IS_ARM(board))
    return (type == SOURCE_TYPE_SPECIAL && index > 0);
  else
    return (type==SOURCE_TYPE_TELEMETRY && (index==TELEMETRY_SOURCE_TX_TIME || index==TELEMETRY_SOURCE_TIMER1 || index==TELEMETRY_SOURCE_TIMER2 || index==TELEMETRY_SOURCE_TIMER3));
}

bool RawSource::isAvailable(const ModelData * const model, const GeneralSettings * const gs, Board::Type board)
{
  if (board == Board::BOARD_UNKNOWN)
    board = getCurrentBoard();

  Boards b(board);

  if (type == SOURCE_TYPE_STICK && index >= b.getCapability(Board::MaxAnalogs))
    return false;

  if (type == SOURCE_TYPE_SWITCH && index >= b.getCapability(Board::Switches))
    return false;

  if (model) {
    if (type == SOURCE_TYPE_VIRTUAL_INPUT && !model->isInputValid(index))
      return false;

    if (type == SOURCE_TYPE_CUSTOM_SWITCH && model->logicalSw[index].isEmpty())
      return false;

    if (type == SOURCE_TYPE_TELEMETRY) {
      if (IS_ARM(board) && !model->sensorData[div(index, 3).quot].isAvailable()) {
        return false;
      }
      else if (!IS_ARM(board)) {
        Firmware * fw = getCurrentFirmware();
        if (type == (int)TELEMETRY_SOURCE_TX_TIME && !fw->getCapability(RtcTime))
          return false;

        if (type == (int)TELEMETRY_SOURCE_SWR && !fw->getCapability(SportTelemetry))
          return false;

        if (type == (int)TELEMETRY_SOURCE_TIMER3 && fw->getCapability(Timers) < 3)
          return false;
      }
    }
  }

  if (gs) {
    int gsIdx = 0;
    if (type == SOURCE_TYPE_STICK && ((isPot(&gsIdx) && !gs->isPotAvailable(gsIdx)) || (isSlider(&gsIdx) && !gs->isSliderAvailable(gsIdx))))
      return false;

    if (type == SOURCE_TYPE_SWITCH && IS_HORUS_OR_TARANIS(board) && !gs->switchSourceAllowedTaranis(index))
      return false;
  }

  if (type == SOURCE_TYPE_TRIM && index >= b.getCapability(Board::NumTrims))
    return false;

  return true;
}

RawSource RawSource::convert(RadioDataConversionState & cstate)
{
  cstate.setItemType("SRC", 1);
  RadioDataConversionState::EventType evt = RadioDataConversionState::EVT_NONE;
  RadioDataConversionState::LogField oldData(index, toString(cstate.fromModel(), cstate.fromGS(), cstate.fromType));

  if (type == SOURCE_TYPE_STICK) {
    if (cstate.toBoard.getCapability(Board::Sliders)) {
      if (index >= cstate.fromBoard.getCapability(Board::Sticks) + cstate.fromBoard.getCapability(Board::Pots)) {
        // 1st slider alignment
        index += cstate.toBoard.getCapability(Board::Pots) - cstate.fromBoard.getCapability(Board::Pots);
      }

      if (isSlider(0, cstate.fromType)) {
        // LS and RS sliders are after 2 aux sliders on X12 and X9E
        if ((IS_HORUS_X12S(cstate.toType) || IS_TARANIS_X9E(cstate.toType)) && !IS_HORUS_X12S(cstate.fromType) && !IS_TARANIS_X9E(cstate.fromType)) {
          if (index >= 7) {
            index += 2;  // LS/RS to LS/RS
          }
        }
        else if (!IS_TARANIS_X9E(cstate.toType) && !IS_HORUS_X12S(cstate.toType) && (IS_HORUS_X12S(cstate.fromType) || IS_TARANIS_X9E(cstate.fromType))) {
          if (index >= 7 && index <= 8) {
            index += 2;   // aux sliders to spare analogs (which may not exist, this is validated later)
            evt = RadioDataConversionState::EVT_CVRT;
          }
          else if (index >= 9 && index <= 10) {
            index -= 2;  // LS/RS to LS/RS
          }
        }
      }
    }

    if (IS_TARANIS(cstate.toType) && IS_HORUS(cstate.fromType)) {
      if (index == 6)
        index = 5;  // pot S2 to S2
      else if (index == 5)
        index = -1;  //  6P on Horus doesn't exist on Taranis
    }
    else  if (IS_HORUS(cstate.toType) && IS_TARANIS(cstate.fromType) && index == 5)
    {
      index = 6;  // pot S2 to S2
    }

  }  // SOURCE_TYPE_STICK

  if (type == SOURCE_TYPE_SWITCH) {
    // SWI to SWR don't exist on !X9E board
    if (!IS_TARANIS_X9E(cstate.toType) && IS_TARANIS_X9E(cstate.fromType)) {
      if (index >= 8) {
        index = index % 8;
        evt = RadioDataConversionState::EVT_CVRT;
      }
    }

    if (IS_TARANIS_X7(cstate.toType) && (IS_TARANIS_X9(cstate.fromType) || IS_HORUS(cstate.fromType))) {
      // No SE and SG on X7 board
      if (index == 4 || index == 6) {
        index = 3;  // SG and SE to SD
        evt = RadioDataConversionState::EVT_CVRT;
      }
      else if (index == 5) {
        index = 4;  // SF to SF
      }
      else if (index == 7) {
        index = 5;  // SH to SH
      }
    }
    // Compensate for SE and SG on X9/Horus board if converting from X7
    else if ((IS_TARANIS_X9(cstate.toType) || IS_HORUS(cstate.toType)) && IS_TARANIS_X7(cstate.fromType)) {
      if (index == 4) {
        index = 5;  // SF to SF
      }
      else if (index == 5) {
        index = 7;  // SH to SH
      }
    }
  }  // SOURCE_TYPE_SWITCH

  // final validation (we do not pass model to isAvailable() because we don't know what has or hasn't been converted)
  if (!isAvailable(NULL, cstate.toGS(), cstate.toType)) {
    cstate.setInvalid(oldData);
    index = -1;  // TODO: better way to flag invalid sources?
    type = MAX_SOURCE_TYPE;
  }
  else if (evt == RadioDataConversionState::EVT_CVRT) {
    cstate.setConverted(oldData, RadioDataConversionState::LogField(index, toString(cstate.toModel(), cstate.toGS(), cstate.toType)));
  }
  else if (oldData.id != index) {
    // provide info by default if anything changed
    cstate.setMoved(oldData, RadioDataConversionState::LogField(index, toString(cstate.toModel(), cstate.toGS(), cstate.toType)));
  }

  return *this;
}
