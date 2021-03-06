#include <stdlib.h>
#include <string.h>
#pragma warning(disable : 4996)

#include "instr.h"

#include "jansson.h"
#include <stdafx.h>
#import "C:\\VNA\\TRVNA\\TRVNA.exe" no_namespace
static ITRVNAPtr pNWA;   // Pointer to COM object of TRVNA.exe
static CComVariant Data; // Variable for measurement data
static CComVariant Freq; // Variable for Frequency points

int instr_connect(json_t *obj)
{
  char err_buff[100] = {'\0'};

  // Init COM subsystem
  if (CoInitialize(NULL) != S_OK)
    strcpy(err_buff, "Unable to initalize COM subsystem.");

  if (pNWA.CreateInstance(__uuidof(TRVNA)) == S_OK)
  {
    if (!pNWA->Ready)
    {
      for (int i = 0; i < 33; i++)
      { // Wait for Hardware to be ready, up to 10 seconds.
        Sleep(300);
        if (pNWA->Ready)
          break;
      }
    }
    if (!pNWA->Ready)
      strcpy(err_buff, "Timeout reached for TRVNA app.");
  }
  else
    strcpy(err_buff, "Unable to open TRVNA app.");
  if (err_buff[0] != '\0')
  {
    json_object_set_new(obj, "instr", json_integer(INSTR_DEV_ERROR));
    json_object_set_new(obj, "err", json_string(err_buff));
    return 0;
  }
  else
  {
    json_object_set_new(obj, "instr", json_integer(INSTR_DEV_READY));
    return 1;
  }
}

void instr_disconnect()
{
  pNWA.Release();
  CoUninitialize();
}

int instr_info(json_t *obj)
{
  json_object_set_new(obj, "instr", json_integer(INSTR_DEV_INFO));
  json_object_set_new(obj, "mode", json_integer(INSTR_MODE_VNA));
  json_object_set_new(obj, "id", json_string((char *)pNWA->NAME));
  return 1;
}

int instr_conf(json_t *obj)
{
  // SET
  for (int i = 0; i < INSTR_MAX_QUERY_SIZE; i++)
  {
    if (instr_queries[i] != NULL)
    {
      const char *key;
      json_t *value;
      json_object_foreach(instr_queries[i], key, value)
      {
        // Stimulus
        if (strcmp(key, "SENS:FREQ:STAR") == 0)
          pNWA->SCPI->SENSe[active_channel]->FREQuency->STARt = atof(json_string_value(value));
        else if (strcmp(key, "SENS:FREQ:STOP") == 0)
          pNWA->SCPI->SENSe[active_channel]->FREQuency->STOP = atof(json_string_value(value));
        else if (strcmp(key, "SENS:SWE:POIN") == 0)
          pNWA->SCPI->SENSe[active_channel]->SWEep->POINts = atol(json_string_value(value));
        else if (strcmp(key, "SENS:BAND:RES") == 0)
          pNWA->SCPI->SENSe[active_channel]->BANDwidth->RESolution = atof(json_string_value(value));
        // Response
        else if (strcmp(key, "CALC:FORM") == 0)
          pNWA->SCPI->CALCulate[active_channel]->SELected->FORMat = json_string_value(value);
        else if (strcmp(key, "CALC:PAR") == 0)
          pNWA->SCPI->CALCulate[active_channel]->PARameter[active_trace]->DEFine = json_string_value(value);
        else if (strcmp(key, "SENS:AVER:COUN") == 0)
          pNWA->SCPI->SENSe[active_channel]->AVERage->COUNt = atol(json_string_value(value));
        else if (strcmp(key, "SENS:SMO:APER") == 0)
          pNWA->SCPI->CALCulate[active_channel]->SELected->SMOothing->APERture = atof(json_string_value(value));

        // Scale
        else if (strcmp(key, "DISP:WIND:TRAC:Y:SCAL:PDIV") == 0)
          pNWA->SCPI->DISPlay->WINDow[active_channel]->TRACe[active_trace]->Y->SCALe->PDIVision = atof(json_string_value(value));
        else if (strcmp(key, "DISP:WIND:TRAC:Y:SCAL:RLEV") == 0)
          pNWA->SCPI->DISPlay->WINDow[active_channel]->TRACe[active_trace]->Y->SCALe->RLEVel = atof(json_string_value(value));
        else if (strcmp(key, "DISP:WIND:TRAC:Y:SCAL:RPOS") == 0)
          pNWA->SCPI->DISPlay->WINDow[active_channel]->TRACe[active_trace]->Y->SCALe->RPOSition = atol(json_string_value(value));
        else if (strcmp(key, "DISP:WIND:Y:SCAL:DIV") == 0)
          pNWA->SCPI->DISPlay->WINDow[active_channel]->Y->SCALe->DIVisions = atol(json_string_value(value));
        else if (strcmp(key, "DISP:WIND:TRAC:Y:SCAL:AUTO") == 0)
          pNWA->SCPI->DISPlay->WINDow[active_channel]->TRACe[active_trace]->Y->SCALe->AUTO();
        // Channels
        else if (strcmp(key, "CALC") == 0)
        {
          if (atol(json_string_value(value)) == 3)
            pNWA->SCPI->DISPlay->SPLit = 4;
          else if (atol(json_string_value(value)) == 4)
            pNWA->SCPI->DISPlay->SPLit = 6;
          else if (atol(json_string_value(value)) == 6)
            pNWA->SCPI->DISPlay->SPLit = 8;
          else if (atol(json_string_value(value)) == 8)
            pNWA->SCPI->DISPlay->SPLit = 9;
          else if (atol(json_string_value(value)) == 9)
            pNWA->SCPI->DISPlay->SPLit = 10;
          else
            pNWA->SCPI->DISPlay->SPLit = atol(json_string_value(value));
          // check active channel
          if (active_channel > atol(json_string_value(value)))
            active_channel = atol(json_string_value(value));
          // check active trace
          if (active_trace > pNWA->SCPI->CALCulate[active_channel]->PARameter[1]->COUNt)
          {
            active_trace = pNWA->SCPI->CALCulate[active_channel]->PARameter[1]->COUNt;
            pNWA->SCPI->CALCulate[active_channel]->PARameter[active_trace]->SELect();
            pNWA->SCPI->DISPlay->WINDow[active_channel]->MAXimize;
          }
        }
        else if (strcmp(key, "CALC:ACT") == 0)
        {
          if (atol(json_string_value(value)) <= channel_count)
            active_channel = atol(json_string_value(value));
          else
            active_channel = channel_count;
          pNWA->SCPI->DISPlay->WINDow[active_channel]->ACTivate();
          pNWA->SCPI->DISPlay->MAXimize;
          // check active trace
          if (active_trace > pNWA->SCPI->CALCulate[active_channel]->PARameter[1]->COUNt)
          {
            active_trace = 1;
            pNWA->SCPI->CALCulate[active_channel]->PARameter[active_trace]->SELect();
            pNWA->SCPI->DISPlay->WINDow[active_channel]->MAXimize;
          }
        }
        else if (strcmp(key, "CALC:PAR:COUN") == 0)
          pNWA->SCPI->CALCulate[active_channel]->PARameter[1]->COUNt = atol(json_string_value(value));
        else if (strcmp(key, "CALC:PAR:SEL") == 0)
        {
          if (atol(json_string_value(value)) <= trace_count)
            active_trace = atol(json_string_value(value));
          else
            active_trace = trace_count;
          pNWA->SCPI->CALCulate[active_channel]->PARameter[atol(json_string_value(value))]->SELect();
          pNWA->SCPI->DISPlay->WINDow[active_channel]->MAXimize;
        }
        // printf("%s: %lu\n", key, atol(json_string_value(value)));
      }
      json_decref(instr_queries[i]);
      instr_queries[i] = NULL;
    }
    else
      break;
  }

  // GET
  json_object_set_new(obj, "instr", json_integer(INSTR_DEV_CONF));
  // Stimulus
  json_object_set_new(obj, "SENS:FREQ:STAR", json_real(pNWA->SCPI->SENSe[active_channel]->FREQuency->STARt));
  json_object_set_new(obj, "SENS:FREQ:STOP", json_real(pNWA->SCPI->SENSe[active_channel]->FREQuency->STOP));
  json_object_set_new(obj, "SENS:SWE:POIN", json_integer(pNWA->SCPI->SENSe[active_channel]->SWEep->POINts));
  json_object_set_new(obj, "SENS:BAND:RES", json_real(pNWA->SCPI->SENSe[active_channel]->BANDwidth->RESolution));
  // Response
  json_object_set_new(obj, "CALC:FORM", json_string(pNWA->SCPI->CALCulate[active_channel]->SELected->FORMat));
  json_object_set_new(obj, "CALC:PAR", json_string(pNWA->SCPI->CALCulate[active_channel]->PARameter[active_trace]->DEFine));
  json_object_set_new(obj, "SENS:AVER:COUN", json_integer(pNWA->SCPI->SENSe[active_channel]->AVERage->COUNt));
  json_object_set_new(obj, "SENS:SMO:APER", json_real(pNWA->SCPI->CALCulate[active_channel]->SELected->SMOothing->APERture));
  // Scale
  json_object_set_new(obj, "DISP:WIND:TRAC:Y:SCAL:PDIV", json_real(pNWA->SCPI->DISPlay->WINDow[active_channel]->TRACe[active_trace]->Y->SCALe->PDIVision));
  json_object_set_new(obj, "DISP:WIND:TRAC:Y:SCAL:RLEV", json_real(pNWA->SCPI->DISPlay->WINDow[active_channel]->TRACe[active_trace]->Y->SCALe->RLEVel));
  json_object_set_new(obj, "DISP:WIND:TRAC:Y:SCAL:RPOS", json_integer(pNWA->SCPI->DISPlay->WINDow[active_channel]->TRACe[active_trace]->Y->SCALe->RPOSition));
  json_object_set_new(obj, "DISP:WIND:Y:SCAL:DIV", json_integer(pNWA->SCPI->DISPlay->WINDow[active_channel]->Y->SCALe->DIVisions));
  // Channels
  long split_mode = pNWA->SCPI->DISPlay->SPLit;
  if (split_mode == 4)
    channel_count = 3;
  else if (split_mode == 6)
    channel_count = 4;
  else if (split_mode == 8)
    channel_count = 6;
  else if (split_mode == 9)
    channel_count = 8;
  else if (split_mode == 10)
    channel_count = 9;
  else
    channel_count = split_mode;
  json_object_set_new(obj, "CALC", json_integer(channel_count));
  active_channel = pNWA->SCPI->SERVice->CHANnel[1]->ACTive;
  json_object_set_new(obj, "CALC:ACT", json_integer(active_channel));
  // Traces
  trace_count = pNWA->SCPI->CALCulate[active_channel]->PARameter[1]->COUNt;
  json_object_set_new(obj, "CALC:PAR:COUN", json_integer(trace_count));
  active_trace = pNWA->SCPI->SERVice->CHANnel[active_channel]->TRACe->ACTive;
  json_object_set_new(obj, "CALC:PAR:SEL", json_integer(active_trace));

  // UI
  json_t *cat, *subcat, *cats, *subcats, *options, *parameters;
  cats = json_array();
  // Stimulus
  cat = json_object();
  json_object_set_new(cat, "name", json_string("STIMULUS"));
  subcats = json_array();

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Start"));
  json_object_set_new(subcat, "scpi", json_string("SENS:FREQ:STAR"));
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Stop"));
  json_object_set_new(subcat, "scpi", json_string("SENS:FREQ:STOP"));
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Points"));
  json_object_set_new(subcat, "scpi", json_string("SENS:SWE:POIN"));
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("IF Bandwidth"));
  json_object_set_new(subcat, "scpi", json_string("SENS:BAND:RES"));
  options = json_array();
  json_array_append_new(options, json_string("30000"));
  json_array_append_new(options, json_string("10000"));
  json_array_append_new(options, json_string("3000"));
  json_array_append_new(options, json_string("1000"));
  json_array_append_new(options, json_string("300"));
  json_array_append_new(options, json_string("100"));
  json_array_append_new(options, json_string("10"));
  json_array_append_new(options, json_string("30"));
  json_object_set_new(subcat, "options", options);
  json_array_append_new(subcats, subcat);

  json_object_set_new(cat, "items", subcats);
  json_array_append_new(cats, cat);
  // Response
  cat = json_object();
  json_object_set_new(cat, "name", json_string("RESPONSE"));
  subcats = json_array();

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Measurement"));
  json_object_set_new(subcat, "scpi", json_string("CALC:PAR"));
  options = json_array();
  json_array_append_new(options, json_string("S11"));
  json_array_append_new(options, json_string("S21"));
  json_array_append_new(options, json_string("A"));
  json_array_append_new(options, json_string("B"));
  json_array_append_new(options, json_string("R"));
  json_object_set_new(subcat, "options", options);
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Format"));
  json_object_set_new(subcat, "scpi", json_string("CALC:FORM"));
  options = json_array();
  json_array_append_new(options, json_string("MLOG"));
  json_array_append_new(options, json_string("PHAS"));
  json_array_append_new(options, json_string("GDEL"));
  json_array_append_new(options, json_string("MLIN"));
  json_array_append_new(options, json_string("SWR"));
  json_array_append_new(options, json_string("REAL"));
  json_array_append_new(options, json_string("IMAG"));
  json_array_append_new(options, json_string("UPH"));
  json_object_set_new(subcat, "options", options);
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Averaging"));
  json_object_set_new(subcat, "scpi", json_string("SENS:AVER:COUN"));
  parameters = json_object();
  json_object_set_new(parameters, "min", json_integer(1));
  json_object_set_new(parameters, "max", json_integer(999));
  json_object_set_new(parameters, "step", json_integer(1));
  json_object_set_new(subcat, "number", parameters);
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Smoothing"));
  json_object_set_new(subcat, "scpi", json_string("SENS:SMO:APER"));
   parameters = json_object();
  json_object_set_new(parameters, "min", json_real(1));
  json_object_set_new(parameters, "max", json_real(20));
  json_object_set_new(parameters, "step", json_real(0.1));
  json_object_set_new(subcat, "number", parameters);
  json_array_append_new(subcats, subcat);

  json_object_set_new(cat, "items", subcats);
  json_array_append_new(cats, cat);
  // Scale
  cat = json_object();
  json_object_set_new(cat, "name", json_string("SCALE"));
  subcats = json_array();

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Scale"));
  json_object_set_new(subcat, "scpi", json_string("DISP:WIND:TRAC:Y:SCAL:PDIV"));
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Ref Value"));
  json_object_set_new(subcat, "scpi", json_string("DISP:WIND:TRAC:Y:SCAL:RLEV"));
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Ref Position"));
  json_object_set_new(subcat, "scpi", json_string("DISP:WIND:TRAC:Y:SCAL:RPOS"));
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Divisions"));
  json_object_set_new(subcat, "scpi", json_string("DISP:WIND:Y:SCAL:DIV"));
  parameters = json_object();
  json_object_set_new(parameters, "min", json_integer(4));
  json_object_set_new(parameters, "max", json_integer(20));
  json_object_set_new(parameters, "step", json_integer(2));
  json_object_set_new(subcat, "number", parameters);
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Auto Scale"));
  json_object_set_new(subcat, "scpi", json_string("DISP:WIND:TRAC:Y:SCAL:AUTO"));
  json_object_set_new(subcat, "button", json_integer(1));
  json_array_append_new(subcats, subcat);

  json_object_set_new(cat, "items", subcats);
  json_array_append_new(cats, cat);
  // Channels
  cat = json_object();
  json_object_set_new(cat, "name", json_string("CHANNELS"));
  subcats = json_array();

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Channel Count"));
  json_object_set_new(subcat, "scpi", json_string("CALC"));
  options = json_array();
  json_array_append_new(options, json_integer(1));
  json_array_append_new(options, json_integer(2));
  json_array_append_new(options, json_integer(3));
  json_array_append_new(options, json_integer(4));
  json_array_append_new(options, json_integer(6));
  json_array_append_new(options, json_integer(8));
  json_array_append_new(options, json_integer(9));
  json_object_set_new(subcat, "options", options);
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Active Channel"));
  json_object_set_new(subcat, "scpi", json_string("CALC:ACT"));
  parameters = json_object();
  json_object_set_new(parameters, "min", json_integer(1));
  json_object_set_new(parameters, "max", json_integer(9));
  json_object_set_new(parameters, "step", json_integer(1));
  json_object_set_new(subcat, "number", parameters);
  json_array_append_new(subcats, subcat);

  json_object_set_new(cat, "items", subcats);
  json_array_append_new(cats, cat);
  // Traces
  cat = json_object();
  json_object_set_new(cat, "name", json_string("TRACES"));
  subcats = json_array();

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Trace Count"));
  json_object_set_new(subcat, "scpi", json_string("CALC:PAR:COUN"));
  parameters = json_object();
  json_object_set_new(parameters, "min", json_integer(1));
  json_object_set_new(parameters, "max", json_integer(8));
  json_object_set_new(parameters, "step", json_integer(1));
  json_object_set_new(subcat, "number", parameters);
  json_array_append_new(subcats, subcat);

  subcat = json_object();
  json_object_set_new(subcat, "name", json_string("Active Trace"));
  json_object_set_new(subcat, "scpi", json_string("CALC:PAR:SEL"));
  parameters = json_object();
  json_object_set_new(parameters, "min", json_integer(1));
  json_object_set_new(parameters, "max", json_integer(8));
  json_object_set_new(parameters, "step", json_integer(1));
  json_object_set_new(subcat, "number", parameters);
  json_array_append_new(subcats, subcat);

  json_object_set_new(cat, "items", subcats);
  json_array_append_new(cats, cat);

  json_object_set_new(obj, "ui", cats);

  return 1;
}

void instr_add_query(json_t *obj)
{
  for (int i = 0; i < INSTR_MAX_QUERY_SIZE; ++i)
  {
    if (instr_queries[i] == NULL)
    {
      instr_queries[i] = obj;
      break;
    }
  }
}

int instr_data(json_t *obj)
{
  json_t *channels = NULL, *traces = NULL, *trace = NULL, *x = NULL, *y1 = NULL, *y2 = NULL;
  json_object_set_new(obj, "instr", json_integer(INSTR_DEV_DATA));

  channels = json_array();
  traces = json_array();
  trace = json_object();

  Data = pNWA->SCPI->CALCulate[active_channel]->SELected->DATA->FDATa;
  Freq = pNWA->SCPI->SENSe[active_channel]->FREQuency->DATA;
  CComSafeArray<double> mSafeArray;
  CComSafeArray<double> FreqArray;
  if (FreqArray.Attach(Freq.parray) == S_OK && mSafeArray.Attach(Data.parray) == S_OK)
  {
    x = json_array();
    y1 = json_array();
    for (unsigned int n = 0; n < FreqArray.GetCount(); n++)
    {
      json_array_append_new(x, json_real(FreqArray.GetAt(n)));
      json_array_append_new(y1, json_real(mSafeArray.GetAt(2 * n)));
    }
    mSafeArray.Detach();
    FreqArray.Detach();
    json_object_set_new(trace, "x", x);
    json_object_set_new(trace, "y1", y1);
  }
  json_array_append_new(traces, trace);
  json_array_append_new(channels, traces);
  json_object_set_new(obj, "channels", channels);
  return 1;
}