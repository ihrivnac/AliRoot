#ifndef ALITPC_EXB_EXACT
#define ALITPC_EXB_EXACT

#include "AliTPCExB.h"
#include "AliFieldMap.h"
#include "AliMagF.h"

class AliTPCExBExact:public AliTPCExB  {
public:
  AliTPCExBExact(const AliFieldMap *bFieldMap,Double_t driftVelocity,
		 Int_t n=100);
  AliTPCExBExact(const AliMagF *bField,Double_t driftVelocity,Int_t n=100,
		 Int_t nx=100,Int_t ny=100,Int_t nz=100);
  virtual ~AliTPCExBExact();
  virtual void Correct(const Double_t *position,Double_t *corrected);
  void TestThisBeautifulObject(const char* fileName);
private:
  AliTPCExBExact& operator=(const AliTPCExBExact&); // don't assign me
  AliTPCExBExact(const AliTPCExBExact&); // don't copy me
  void CreateLookupTable();
  void GetE(Double_t *E,const Double_t *x) const;
  void GetB(Double_t *B,const Double_t *x) const;
  void Motion(const Double_t *x,Double_t t,Double_t *dxdt) const;
  void CalculateDistortion(const Double_t *x,Double_t *dist) const;
  void DGLStep(Double_t *x,Double_t t,Double_t h) const;
  const AliFieldMap *fkMap; // the magnetic field map as supplied by the user
  const AliMagF *fkField;   // the magnetic field as supplied by the user
  Int_t fkN;       // max number of integration steps
  Int_t fkNX;      // field mesh points in x direction
  Int_t fkNY;      // field mesh points in y direction
  Int_t fkNZ;      // field mesh points in z direction
  Double_t fkXMin; // the first grid point in x direction
  Double_t fkXMax; // the last grid point in x direction
  Double_t fkYMin; // the first grid point in y direction
  Double_t fkYMax; // the last grid point in y direction
  Double_t fkZMin; // the first grid point in z direction
  Double_t fkZMax; // the last grid point in z direction
  Double_t *fLook; // the great lookup table
  static const Double_t fgkEM; // elementary charge over electron mass (C/kg)
  static const Double_t fgkDriftField; // the TPC drift field (V/m) (modulus)

  ClassDef(AliTPCExBExact,1)
};

#endif
