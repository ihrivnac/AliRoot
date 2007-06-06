#ifndef ALITPC_EXB_FIRST_H
#define ALITPC_EXB_FIRST_H

#include "AliTPCExB.h"
#include "AliFieldMap.h"
#include "AliMagF.h"

class AliTPCExBFirst:public AliTPCExB {
public:
  AliTPCExBFirst(const AliFieldMap *bFieldMap,Double_t driftVelocity);
  AliTPCExBFirst(const AliMagF *bField,Double_t driftVelocity,
		 Int_t nx=100,Int_t ny=100,Int_t nz=100);
  virtual ~AliTPCExBFirst();
  virtual void Correct(const Double_t *position,Double_t *corrected);
  void TestThisBeautifulObject(const char* fileName);
private:
  AliTPCExBFirst& operator=(const AliTPCExBFirst&); // don't assign me
  AliTPCExBFirst(const AliTPCExBFirst&); // don't copy me
  void ConstructCommon(const AliFieldMap *bFieldMap,const AliMagF *bField);
  void GetMeanFields(Double_t rx,Double_t ry,Double_t rz,
		     Double_t *Bx,Double_t *By) const;
  Int_t fkNX;        // field mesh points in x direction
  Int_t fkNY;        // field mesh points in y direction
  Int_t fkNZ;        // field mesh points in z direction
  Double_t fkXMin;   // the first grid point in x direction
  Double_t fkXMax;   // the last grid point in x direction
  Double_t fkYMin;   // the first grid point in y direction
  Double_t fkYMax;   // the last grid point in y direction
  Double_t fkZMin;   // the first grid point in z direction
  Double_t fkZMax;   // the last grid point in z direction
  Double_t *fMeanBx; // the mean field in x direction upto a certain z value
  Double_t *fMeanBy; // the mean field in y direction upto a certain z value
  Double_t fMeanBz;  // the mean field in z direction inside the TPC volume
  static const Double_t fgkEM; // elementary charge over electron mass (C/kg)
  static const Double_t fgkDriftField; // the TPC drift field (V/m) (modulus)

  ClassDef(AliTPCExBFirst,1)
};

#endif
