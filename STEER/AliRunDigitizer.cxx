/**************************************************************************
* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
*                                                                        *
* Author: The ALICE Off-line Project.                                    *
* Contributors are mentioned in the code where appropriate.              *
*                                                                        *
* Permission to use, copy, modify and distribute this software and its   *
* documentation strictly for non-commercial purposes is hereby granted   *
* without fee, provided that the above copyright notice appears in all   *
* copies and that both the copyright notice and this permission notice   *
* appear in the supporting documentation. The authors make no claims     *
* about the suitability of this software for any purpose. It is          *
* provided "as is" without express or implied warranty.                  *
**************************************************************************/

/* $Id$ */

//_______________________________________________________________________
//
// AliRunDigitizer.cxx
//
// Manager object for merging/digitization
//
// Instance of this class manages the digitization and/or merging of
// Sdigits into Digits. 
//
// Only one instance of this class is created in the macro:
//   AliRunDigitizer * manager = 
//      new AliRunDigitizer(nInputStreams,SPERB);
// where nInputStreams is number of input streams and SPERB is
// signals per background variable, which determines how combinations
// of signal and background events are generated.
// Then instances of specific detector digitizers are created:
//   AliMUONDigitizer *dMUON  = new AliMUONDigitizer(manager)
// and the I/O configured (you have to specify input files 
// and an output file). The manager connects appropriate trees from 
// the input files according a combination returned by AliMergeCombi 
// class. It creates TreeD in the output and runs once per 
// event Digitize method of all existing AliDetDigitizers 
// (without any option). AliDetDigitizers ask manager
// for a TTree with input (manager->GetInputTreeS(Int_t i),
// merge all inputs, digitize it, and save it in the TreeD 
// obtained by manager->GetTreeD(). Output events are stored with 
// numbers from 0, this default can be changed by 
// manager->SetFirstOutputEventNr(Int_t) method. The particle numbers
// in the output are shifted by MASK, which is taken from manager.
//
// The default output is to the signal file (stream 0). This can be 
// changed with the SetOutputFile(TString fn)  method.
//
// Single input file is permitted. Maximum kMaxStreamsToMerge can be merged.
// Input from the memory (on-the-fly merging) is not yet 
// supported, as well as access to the input data by invoking methods
// on the output data.
//
// Access to the some data is via gAlice for now (supposing the 
// same geometry in all input files), gAlice is taken from the first 
// input file on the first stream.
//
// Example with MUON digitizer, no merging, just digitization
//
//  AliRunDigitizer * manager = new AliRunDigitizer(1,1);
//  manager->SetInputStream(0,"galice.root");
//  AliMUONDigitizer *dMUON  = new AliMUONDigitizer(manager);
//  manager->Exec("");
//
// Example with MUON digitizer, merge all events from 
//   galice.root (signal) file with events from bgr.root 
//   (background) file. Number of merged events is
//   min(number of events in galice.root, number of events in bgr.root)
//
//  AliRunDigitizer * manager = new AliRunDigitizer(2,1);
//  manager->SetInputStream(0,"galice.root");
//  manager->SetInputStream(1,"bgr.root");
//  AliMUONDigitizer *dMUON  = new AliMUONDigitizer(manager);
//  manager->Exec("");
//
// Example with MUON digitizer, save digits in a new file digits.root,
//   process only 1 event
//
//  AliRunDigitizer * manager = new AliRunDigitizer(2,1);
//  manager->SetInputStream(0,"galice.root");
//  manager->SetInputStream(1,"bgr.root");
//  manager->SetOutputFile("digits.root");
//  AliMUONDigitizer *dMUON  = new AliMUONDigitizer(manager);
//  manager->SetNrOfEventsToWrite(1);
//  manager->Exec("");
//
//_______________________________________________________________________ 

// system includes

#include <Riostream.h>

// ROOT includes

#include "TFile.h"
#include "TList.h"
#include "TTree.h"

// AliROOT includes

#include "AliDigitizer.h"
#include "AliMergeCombi.h"
#include "AliRunLoader.h"
#include "AliLoader.h"
#include "AliRun.h"
#include "AliRunDigitizer.h"
#include "AliStream.h"
#include "AliHeader.h"

ClassImp(AliRunDigitizer)

const TString AliRunDigitizer::fgkDefOutFolderName("Output");
const TString AliRunDigitizer::fgkBaseInFolderName("Input");


//_______________________________________________________________________
AliRunDigitizer::AliRunDigitizer():
 fkMASKSTEP(0),
 fOutputFileName(0),
 fOutputDirName(0),
 fEvent(0),
 fNrOfEventsToWrite(0),
 fNrOfEventsWritten(0),
 fCopyTreesFromInput(0),
 fNinputs(0),
 fNinputsGiven(0),
 fInputStreams(0x0),
 fOutRunLoader(0x0),
 fOutputInitialized(kFALSE),
 fCombi(0),
 fCombination(0),
 fCombinationFileName(0),
 fDebug(0)
{
// root requires default ctor, where no new objects can be created
// do not use this ctor, it is supplied only for root needs

//  fOutputStream = 0x0;
}

//_______________________________________________________________________
AliRunDigitizer::AliRunDigitizer(Int_t nInputStreams, Int_t sperb):
 TTask("AliRunDigitizer","The manager for Merging"),
 fkMASKSTEP(10000000),
 fOutputFileName(""),
 fOutputDirName("."),
 fEvent(0),
 fNrOfEventsToWrite(-1),
 fNrOfEventsWritten(0),
 fCopyTreesFromInput(-1),
 fNinputs(nInputStreams),
 fNinputsGiven(0),
 fInputStreams(new TClonesArray("AliStream",nInputStreams)),
 fOutRunLoader(0x0),
 fOutputInitialized(kFALSE),
 fCombi(new AliMergeCombi(nInputStreams,sperb)),
 fCombination(kMaxStreamsToMerge),
 fCombinationFileName(0),
 fDebug(0)

{
// ctor which should be used to create a manager for merging/digitization
  if (nInputStreams == 0) 
   {//kidding
    Fatal("AliRunDigitizer","Specify nr of input streams");
    return;
   }
  Int_t i;
  
  fkMASK[0] = 0;
  
  for (i=1;i<kMaxStreamsToMerge;i++) 
   {
    fkMASK[i] = fkMASK[i-1] + fkMASKSTEP;
   }
  
  TClonesArray &lInputStreams = *fInputStreams;
  
  for (i=0;i<nInputStreams;i++) {
    new(lInputStreams[i]) AliStream(fgkBaseInFolderName+(Long_t)i,"UPDATE");
  }
}
//_______________________________________________________________________

AliRunDigitizer::AliRunDigitizer(const AliRunDigitizer& dig):
 TTask(dig),
 fkMASKSTEP(0),
 fOutputFileName(0),
 fOutputDirName(0),
 fEvent(0),
 fNrOfEventsToWrite(0),
 fNrOfEventsWritten(0),
 fCopyTreesFromInput(0),
 fNinputs(0),
 fNinputsGiven(0),
 fInputStreams(0x0),
 fOutRunLoader(0x0),
 fOutputInitialized(kFALSE),
 fCombi(0),
 fCombination(0),
 fCombinationFileName(0),
 fDebug(0)
{
  //
  // Copy ctor
  //
  dig.Copy(*this);
}
//_______________________________________________________________________

void AliRunDigitizer::Copy(TObject&) const
{
  Fatal("Copy","Not installed\n");
}

//_______________________________________________________________________

AliRunDigitizer::~AliRunDigitizer() {
// dtor
  if (GetListOfTasks()) 
    GetListOfTasks()->Clear("nodelete");
  delete fInputStreams;
  delete fCombi;
  delete fOutRunLoader;
}
//_______________________________________________________________________
void AliRunDigitizer::AddDigitizer(AliDigitizer *digitizer)
{
// add digitizer to the list of active digitizers
  this->Add(digitizer);
}
//_______________________________________________________________________
void AliRunDigitizer::SetInputStream(Int_t i, const char *inputFile, TString foldername)
{
//
// Sets the name of the input file
//
  if (i > fInputStreams->GetLast()) {
    Error("SetInputStream","Input stream number too high");
    return;
  }
  AliStream * stream = static_cast<AliStream*>(fInputStreams->At(i)) ; 
  if ( !foldername.IsNull() ) {
    if ( i > 0 ) 
      foldername += i ; // foldername should stay unchanged for the default output 
    stream->SetFolderName(foldername) ;
  } 
  stream->AddFile(inputFile);
}

//_______________________________________________________________________
void AliRunDigitizer::Digitize(Option_t* option)
{
// get a new combination of inputs, loads events to folders

// take gAlice from the first input file. It is needed to access
//  geometry data
// If gAlice is already in memory, use it
  SetDebug(10);
  
  if (gAlice == 0x0)
   {
    if (!static_cast<AliStream*>(fInputStreams->At(0))->ImportgAlice()) 
     {
       Error("Digitize","Error occured while getting gAlice from Input 0");
       return;
     }
   }
    
  if (!InitGlobal()) //calls Init() for all (sub)digitizers
   {
     Error("Digitize","InitGlobal returned error");
     return;
   }
   
  Int_t eventsCreated = 0;
// loop until there is anything on the input in case fNrOfEventsToWrite < 0
  while ((eventsCreated++ < fNrOfEventsToWrite) || (fNrOfEventsToWrite < 0)) 
   {
      if (!ConnectInputTrees()) break;
      InitEvent();//this must be after call of Connect Input tress.
      if (fOutRunLoader)
       {
         fOutRunLoader->SetEventNumber(eventsCreated-1);
       }
      static_cast<AliStream*>(fInputStreams->At(0))->ImportgAlice(); // use gAlice of the first input stream
      ExecuteTasks(option);// loop over all registered digitizers and let them do the work
      FinishEvent();
      CleanTasks();
   }
  FinishGlobal();
}

//_______________________________________________________________________
Bool_t AliRunDigitizer::ConnectInputTrees()
{
//loads events 
  Int_t eventNr[kMaxStreamsToMerge], delta[kMaxStreamsToMerge];
  fCombi->Combination(eventNr, delta);
  for (Int_t i=0;i<fNinputs;i++) 
   {
    if (delta[i] == 1)
     {
      AliStream *iStream = static_cast<AliStream*>(fInputStreams->At(i));//gets the "i" defined  in combination
      if (!iStream->NextEventInStream()) return kFALSE; //sets serial number
     } 
    else if (delta[i] != 0) 
     {
      Error("ConnectInputTrees","Only delta 0 or 1 is implemented");
      return kFALSE;
     }
   }

  return kTRUE;
}

//_______________________________________________________________________
Bool_t AliRunDigitizer::InitGlobal()
{
// called once before Digitize() is called, initialize digitizers and output
  fOutputInitialized = kFALSE;
  TList* subTasks = this->GetListOfTasks();
  if (subTasks) {
    TIter next(subTasks);
    while (AliDigitizer * dig = (AliDigitizer *) next())
     dig->Init();
  }
  return kTRUE;
}

//_______________________________________________________________________

void AliRunDigitizer::SetOutputFile(TString fn)
{
// the output will be to separate file, not to the signal file
 //here should be protection to avoid setting the same file as any input 
  Info("SetOutputFile","Setting Output File Name %s ",fn.Data());
  fOutputFileName = fn;
//  InitOutputGlobal();
}

//_______________________________________________________________________
Bool_t AliRunDigitizer::InitOutputGlobal()
{
// Creates the output file, called by InitEvent()
//Needs to be called after all inputs are opened  
  if (fOutputInitialized) return kTRUE;
  
  if ( !fOutputFileName.IsNull())
   {
    fOutRunLoader = AliRunLoader::Open(fOutputFileName,fgkDefOutFolderName,"recreate");
    
    if (fOutRunLoader == 0x0)
     {
       Error("InitOutputGlobal","Can not open ooutput");
       return kFALSE;
     }
    Info("InitOutputGlobal", " 1 %s = ", GetInputFolderName(0).Data()) ; 
    AliRunLoader* inrl = AliRunLoader::GetRunLoader(GetInputFolderName(0));
    if (inrl == 0x0)
     {
       Error("InitOutputGlobal","Can not get Run Loader Input 0. Maybe yet not initialized?");
       return kFALSE;
     }
    Info("InitOutputGlobal", " 2 %#x = ", inrl) ; 

    //Copy all detector loaders from input 0 to output
    const TObjArray* inloaders = inrl->GetArrayOfLoaders();
    TIter next(inloaders);
    AliLoader *loader;
    while((loader = (AliLoader*)next()))
     {
       AliLoader* cloneloader = (AliLoader*)loader->Clone();
       cloneloader->Register(fOutRunLoader->GetEventFolder());//creates folders for this loader in Output Folder Structure
       GetOutRunLoader()->AddLoader(cloneloader);//adds a loader for output
     }

    fOutRunLoader->MakeTree("E");
    
    if (GetDebug()>2)  Info("InitOutputGlobal","file %s was opened.",fOutputFileName.Data());
   }
  fOutputInitialized = kTRUE; 
  return kTRUE;
}
//_______________________________________________________________________

void AliRunDigitizer::InitEvent()
{
//redirects output properly
  if (GetDebug()>2)
   {
    Info("InitEvent","fEvent = %d",fEvent);
    Info("InitEvent","fOutputFileName \"%s\"",fOutputFileName.Data());
   }
  if (fOutputInitialized == kFALSE) InitOutputGlobal();
  
// if fOutputFileName was not given, write output to signal directory
}
//_______________________________________________________________________

void AliRunDigitizer::FinishEvent()
{
// called at the end of loop over digitizers

  
  if (GetOutRunLoader() == 0x0)
   {
     Error("FinishEvent","fOutRunLoader is null");
     return;
   }
  
  fEvent++;
  fNrOfEventsWritten++;
  
  if (fOutRunLoader)
  {
     AliRunLoader* inrl = AliRunLoader::GetRunLoader(GetInputFolderName(0));
     AliHeader* outheader = fOutRunLoader->GetHeader();
     AliHeader* inheader = inrl->GetHeader();
     if (inheader == 0x0)
     {
       inrl->LoadHeader();
       inheader = inrl->GetHeader();
       if (inheader == 0x0) Fatal("FinishEvent","Can not get header from input 0");
     }
     
     outheader->SetNprimary(inheader->GetNprimary());
     outheader->SetNtrack(inheader->GetNtrack());
     outheader->SetEvent(inheader->GetEvent());
     outheader->SetEventNrInRun(inheader->GetEventNrInRun());
     outheader->SetNvertex(inheader->GetNvertex());
     fOutRunLoader->TreeE()->Fill();
  }
      
  if (fCopyTreesFromInput > -1) 
   {
    //this is sensless since no information would be coherent in case of merging
    //
    cout<<"Copy trees from input: Copy or link files manually"<<endl;
    return;
   }
}
//_______________________________________________________________________

void AliRunDigitizer::FinishGlobal()
{
// called at the end of Exec
// save unique objects to the output file

  if (GetOutRunLoader() == 0x0)
   {
     Error("FinishGlobal","Can not get RunLoader from Output Stream folder");
     return;
   }
  TFile* file = TFile::Open(fOutputDirName + "/digitizer.root", "recreate");
  this->Write();
  file->Close();
  delete file;
  if (fOutRunLoader)
   {
     fOutRunLoader->WriteHeader("OVERWRITE");
     fOutRunLoader->WriteRunLoader("OVERWRITE");
     TFolder* outfolder = fOutRunLoader->GetEventFolder();
     if (outfolder == 0x0)
     {
       Error("FinishEvent","Can not get Event Folder");
       return;
     }    
  
     AliRunLoader* inRN = AliRunLoader::GetRunLoader(GetInputFolderName(0));
     outfolder->Add(inRN->GetAliRun());
     fOutRunLoader->WriteAliRun("OVERWRITE");
   }
   
  if (fCopyTreesFromInput > -1) 
   {
     //copy files manually
   }
}
//_______________________________________________________________________

Int_t  AliRunDigitizer::GetNParticles(Int_t event) const
{
// return number of particles in all input files for a given
// event (as numbered in the output file)
// return -1 if some file cannot be accessed

  Int_t sum = 0;
  Int_t sumI;
  for (Int_t i = 0; i < fNinputs; i++) {
    sumI = GetNParticles(GetInputEventNumber(event,i), i);
    if (sumI < 0) return -1;
    sum += sumI;
  }
  return sum;
}
//_______________________________________________________________________

Int_t  AliRunDigitizer::GetNParticles(Int_t /*event*/, Int_t /*input*/) const
{
// return number of particles in input file input for a given
// event (as numbered in this input file)
// return -1 if some error

// Must be revised in the version with AliStream

  return -1;

}

//_______________________________________________________________________
Int_t* AliRunDigitizer::GetInputEventNumbers(Int_t event) const
{
// return pointer to an int array with input event numbers which were
// merged in the output event event

// simplified for now, implement later
  Int_t * a = new Int_t[kMaxStreamsToMerge];
  for (Int_t i = 0; i < fNinputs; i++) {
    a[i] = event;
  }
  return a;
}
//_______________________________________________________________________
Int_t AliRunDigitizer::GetInputEventNumber(Int_t event, Int_t /*input*/) const
{
// return an event number of an eventInput from input file input
// which was merged to create output event event

// simplified for now, implement later
  return event;
}
//_______________________________________________________________________
TParticle* AliRunDigitizer::GetParticle(Int_t i, Int_t event) const
{
// return pointer to particle with index i (index with mask)

// decode the MASK
  Int_t input = i/fkMASKSTEP;
  return GetParticle(i,input,GetInputEventNumber(event,input));
}

//_______________________________________________________________________
TParticle* AliRunDigitizer::GetParticle(Int_t /*i*/, Int_t /*input*/, Int_t /*event*/) const
{
// return pointer to particle with index i in the input file input
// (index without mask)
// event is the event number in the file input
// return 0 i fit does not exist

// Must be revised in the version with AliStream

  return 0;
}

//_______________________________________________________________________
void AliRunDigitizer::ExecuteTask(Option_t* option)
{
// overwrite ExecuteTask to do Digitize only

  if (!IsActive()) return;
  Digitize(option);
  fHasExecuted = kTRUE;
  return;
}

//_______________________________________________________________________
const TString& AliRunDigitizer::GetInputFolderName(Int_t i) const
{
  AliStream* stream = dynamic_cast<AliStream*>(fInputStreams->At(i));
  if (stream == 0x0)
   {
     Fatal("GetInputFolderName","Can not get the input stream. Index = %d. Exiting",i);
   }
  return stream->GetFolderName();
}
//_______________________________________________________________________

const char* AliRunDigitizer::GetOutputFolderName()
{
  return GetOutRunLoader()->GetEventFolder()->GetName();
}
//_______________________________________________________________________

AliRunLoader* AliRunDigitizer::GetOutRunLoader()
{
  if (fOutRunLoader) return fOutRunLoader;
  
  if ( fOutputFileName.IsNull() )
   {//guard that sombody calls it without settting file name
    cout<<"Output file name is empty. Using Input 0 for output\n";
    return AliRunLoader::GetRunLoader(GetInputFolderName(0));
   }
//  InitOutputGlobal();
  return fOutRunLoader;
}
//_______________________________________________________________________

TString AliRunDigitizer::GetInputFileName(Int_t input, Int_t order) const 
{
// returns file name of the order-th file in the input stream input
// returns empty string if such file does not exist
// first input stream is 0
// first file in the input stream is 0
  TString fileName("");
  if (input >= fNinputs) return fileName;
  AliStream * stream = static_cast<AliStream*>(fInputStreams->At(input));
  if (order > stream->GetNInputFiles()) return fileName;
  fileName = stream->GetFileName(order);
  return fileName;
}
