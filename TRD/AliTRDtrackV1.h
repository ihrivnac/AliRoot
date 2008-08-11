#ifndef ALITRDTRACKV1_H
#define ALITRDTRACKV1_H
/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

/* $Id$ */

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//  Represents a reconstructed TRD track                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifndef ALIKALMANTRACK_H
#include "AliKalmanTrack.h"
#endif

#ifndef ALIESDTRACK_H
#include "AliESDtrack.h"
#endif

#ifndef ALITRDSEEDV1_H
#include "AliTRDseedV1.h"
#endif

class AliTRDcluster;
class AliESDtrack;
class AliTRDReconstructor;
class AliTRDtrackV1 : public AliKalmanTrack
{
public:
  enum { kMAXCLUSTERSPERTRACK = 210 };
    
  enum { kNdet      = 540
        , kNstacks   =  90
        , kNplane    =   AliESDtrack::kTRDnPlanes
        , kNcham     =   5
        , kNsect     =  18
        , kNslice    =   3
        , kNMLPslice =   8 };
  
  enum AliTRDtrackStatus {
          kOwner   = BIT(1)
        , kStopped = BIT(2) 
  };

  AliTRDtrackV1();
  AliTRDtrackV1(AliTRDseedV1 *trklts, const Double_t p[5], const Double_t cov[15], Double_t x, Double_t alpha);
  AliTRDtrackV1(const AliESDtrack &ref);
  AliTRDtrackV1(const AliTRDtrackV1 &ref);
  virtual ~AliTRDtrackV1();
  AliTRDtrackV1 &operator=(const AliTRDtrackV1 &ref) { *(new(this) AliTRDtrackV1(ref)); return *this; }
  
  Bool_t         CookPID();
  Bool_t         CookLabel(Float_t wrong);
  AliTRDtrackV1* GetBackupTrack() const {return fBackupTrack;}
  Double_t       GetBudget(Int_t i) const { return fBudget[i];}
  Double_t       GetC() const { return AliExternalTrackParam::GetC(GetBz());}
  Int_t          GetClusterIndex(Int_t id) const;
  Float_t        GetEdep() const {return fDE;}
  inline Float_t GetMomentum(Int_t plane) const;
  inline Int_t   GetNCross();
  inline Int_t   GetNumberOfTracklets() const;
  Double_t       GetPIDsignal() const {return 0.;}
  Double_t       GetPID(Int_t is) const { return (is >=0 && is < AliPID::kSPECIES) ? fPID[is] : -1.;}
  Double_t       GetPredictedChi2(const AliTRDseedV1 *tracklet) const;
  Double_t       GetPredictedChi2(const AliCluster* /*c*/) const                   { return 0.0; }
  Int_t          GetProlongation(Double_t xk, Double_t &y, Double_t &z);
  AliTRDseedV1*  GetTracklet(Int_t plane) const {return plane >=0 && plane <kNplane ? fTracklet[plane] : 0x0;}
  Int_t          GetTrackletIndex(Int_t plane) const          { return (plane>=0 && plane<kNplane) ? fTrackletIndex[plane] : -1;} 
  UShort_t*      GetTrackletIndexes() {return &fTrackletIndex[0];}
  
  Bool_t         IsOwner() const {return TestBit(kOwner);};
  Bool_t         IsStopped() const {return TestBit(kStopped);};
  
  void           MakeBackupTrack();
  Bool_t         PropagateTo(Double_t xr, Double_t x0 = 8.72, Double_t rho = 5.86e-3);
  Int_t          PropagateToR(Double_t xr, Double_t step);
  Bool_t         Rotate(Double_t angle, Bool_t absolute = kFALSE);
  void           SetBudget(Int_t i, Double_t b) {if(i>=0 && i<3) fBudget[i] = b;}
  void           SetNumberOfClusters();
  void           SetOwner();
  void           SetStopped(Bool_t stop) {SetBit(kStopped, stop);}
  void           SetTracklet(AliTRDseedV1 *trklt,  Int_t index);
  inline void    SetReconstructor(const AliTRDReconstructor *rec);
  inline Float_t StatusForTOF();
  Bool_t         Update(AliTRDseedV1 *tracklet, Double_t chi2);
  //Bool_t         Update(const AliTRDcluster *c, Double_t chi2, Int_t index, Double_t h01){ return AliTRDtrack::Update(c,chi2,index,h01); };
  Bool_t         Update(const AliCluster *, Double_t, Int_t)                        { return kFALSE; };
  void           UpdateESDtrack(AliESDtrack *t);

protected:
  Double_t       GetBz() const;

private:
  UChar_t      fPIDquality;           //  No of planes used for PID calculation	
  UShort_t     fTrackletIndex[kNplane];//  Tracklets index in the tracker list
  Double32_t   fPID[AliPID::kSPECIES];//  PID probabilities
  Double32_t   fBudget[3];            //  Integrated material budget
  Double32_t   fDE;                   //  Integrated delta energy
  AliTRDseedV1 *fTracklet[kNplane];   //  Tracklets array defining the track
  AliTRDtrackV1 *fBackupTrack;        // Backup track


  ClassDef(AliTRDtrackV1, 2)          // new TRD track
};

//____________________________________________________
inline Float_t AliTRDtrackV1::GetMomentum(Int_t plane) const
{
  return plane >=0 && plane < kNplane && fTrackletIndex[plane] != 0xff ? fTracklet[plane]->GetMomentum() : -1.;
}

//____________________________________________________
inline Int_t AliTRDtrackV1::GetNCross()
{
  Int_t ncross = 0;
  for(Int_t ip=0; ip<kNplane; ip++){
    if(!fTracklet[ip]) continue;
    ncross += fTracklet[ip]->IsRowCross();
  }
  return ncross;
}

//____________________________________________________
inline Int_t AliTRDtrackV1::GetNumberOfTracklets() const
{
  Int_t n = 0;
  for(Int_t ip=0; ip<kNplane; ip++){
    if(!fTracklet[ip]) continue;
    n++;
  }
  return n;
}


//____________________________________________________
inline void AliTRDtrackV1::SetReconstructor(const AliTRDReconstructor *rec)
{
  for(Int_t ip=0; ip<kNplane; ip++){
    if(!fTracklet[ip]) continue;
    fTracklet[ip]->SetReconstructor(rec);
  }
}


//____________________________________________________________________________
inline Float_t AliTRDtrackV1::StatusForTOF()
{
  //
  // Defines the status of the TOF extrapolation
  //

  if(!fTracklet[5]) return 0.;

  // Definition of res ????
  Float_t res = /*(0.2 + 0.8 * (fN / (fNExpected + 5.0))) **/ (0.4 + 0.6 * fTracklet[5]->GetN() / 20.0);
  res *= (0.25 + 0.8 * 40.0 / (40.0 + fBudget[2]));
  return res;
}

#endif



