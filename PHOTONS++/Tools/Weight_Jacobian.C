#include "PHOTONS++/Tools/Weight_Jacobian.H"
#include "ATOOLS/Phys/Particle.H"
#include "ATOOLS/Math/MathTools.H"
#include "ATOOLS/Org/Message.H"

using namespace ATOOLS;
using namespace PHOTONS;
using namespace std;


// class Weight_Jacobian
Weight_Jacobian::Weight_Jacobian() {
}

Weight_Jacobian::~Weight_Jacobian() {
}

Vec4D Weight_Jacobian::CalculateMomentumSum(const Particle_Vector& pv) {
  Vec4D sum = Vec4D(0.,0.,0.,0.);
  for (unsigned int i=0; i<pv.size(); i++) sum+=pv[i]->Momentum();
  return sum;
}


// class Weight_Jacobian_Lorentz
Weight_Jacobian_Lorentz::Weight_Jacobian_Lorentz
(const Particle_Vector& newdip, const Particle_Vector& newspec,
 const Particle_Vector& olddip, const Particle_Vector& oldspec,
 const Particle_Vector& phot, Dipole_Type::code dtype) {
  m_dtype   = dtype;
  if (m_dtype == Dipole_Type::ff) {
    // only final state dipole constituents
    m_QC0      = CalculateMomentumSum(olddip)[0];
    m_PC0      = CalculateMomentumSum(newdip)[0];
    m_mMQ   = m_QC0;
    m_mMP   = m_PC0;
  }
  else if (m_dtype == Dipole_Type::fi) {
    // initial state dipole constituent at position 0
    m_QC0      = (CalculateMomentumSum(olddip) - olddip[0]->Momentum())[0];
    m_PC0      = (CalculateMomentumSum(newdip) - newdip[0]->Momentum())[0];
    m_mMQ   = CalculateMomentumSum(olddip)[0];
    m_mMP   = CalculateMomentumSum(newdip)[0];
  }
  m_QN0     = CalculateMomentumSum(oldspec)[0];
  m_PN0     = CalculateMomentumSum(newspec)[0];
  m_K0      = CalculateMomentumSum(phot)[0];
  CalculateWeight();
  CalculateMax();
}

Weight_Jacobian_Lorentz::~Weight_Jacobian_Lorentz() {
}

void Weight_Jacobian_Lorentz::CalculateWeight() {
  m_weight = (m_mMP*m_mMP*m_mMP)/(m_mMQ*m_mMQ*m_mMQ)
                * (m_QC0+m_QN0)/(m_PC0 + m_PN0 + m_K0);
}

void Weight_Jacobian_Lorentz::CalculateMax() {
  m_maxweight = 1.;
}



// class Weight_Jacobian_Mapping
Weight_Jacobian_Mapping::Weight_Jacobian_Mapping
(const Particle_Vector& newdip, const Particle_Vector& newspec,
 const Particle_Vector& olddip, const Particle_Vector& oldspec,
 const Vec4D& phot, double M, double u, Dipole_Type::code dtype) {
  m_dtype         = dtype;
  m_newdipole     = newdip;
  m_olddipole     = olddip;
  m_newspectator  = newspec;
  m_oldspectator  = oldspec;
  m_K             = Vec3D(phot);
  m_M             = M;
  m_u             = u;
  CalculateWeight();
  CalculateMax();
}

Weight_Jacobian_Mapping::~Weight_Jacobian_Mapping() {
}

void Weight_Jacobian_Mapping::CalculateWeight() {
  DEBUG_FUNC("");
  Vec3D Q         = Vec3D(0.,0.,0.);
  Vec3D P         = Vec3D(0.,0.,0.);
  Vec3D QN        = Vec3D(0.,0.,0.);
  Vec3D PN        = Vec3D(0.,0.,0.);
  double sumq     = 0.;
  double sump     = 0.;
  double prod     = 1.;
  int N           = m_newdipole.size() + m_newspectator.size();
  // for Q -> sum_i^n q_i corrected to P -> sum_i p_i + gamma(s)
  // w = u^(3n-4)
  //     * (vec(P)^2/P^0 - sum_1^n vec(p_i)^2/p_i^0)/
  //       (vec(P)*vec(Q)/P^0 - sum_1^n vec(p_i)vec(q_i)/p_i^0)
  //     * prod_1^n (q_i^0/p_i^0)
  // sum over final state particles
  for (unsigned int i=0; i<m_newdipole.size(); i++) {
    // exclude initial state particle (in initial-final dipoles at position 0)
    if (!((m_dtype == Dipole_Type::fi) && (i == 0))) {
      sumq = sumq - Vec3D(m_olddipole[i]->Momentum()).Sqr()
                      /m_olddipole[i]->Momentum()[0];
      sump = sump - Vec3D(m_newdipole[i]->Momentum())
                      *Vec3D(m_olddipole[i]->Momentum())
                      /m_newdipole[i]->Momentum()[0];
      prod = prod * m_olddipole[i]->Momentum()[0]
                      /m_newdipole[i]->Momentum()[0];
      Q = Q + Vec3D(m_olddipole[i]->Momentum());
      P = P + Vec3D(m_newdipole[i]->Momentum());
    }
  }
  for (unsigned int i=0; i<m_newspectator.size(); i++) {
    sumq = sumq - Vec3D(m_oldspectator[i]->Momentum()).Sqr()
                    /m_oldspectator[i]->Momentum()[0];
    sump = sump - Vec3D(m_newspectator[i]->Momentum())
                    *Vec3D(m_oldspectator[i]->Momentum())
                    /m_newspectator[i]->Momentum()[0];
    prod = prod * m_oldspectator[i]->Momentum()[0]
                    /m_newspectator[i]->Momentum()[0];
    QN = QN + Vec3D(m_oldspectator[i]->Momentum());
    PN = PN + Vec3D(m_newspectator[i]->Momentum());
  }
  // add initial state term
  if (m_dtype == Dipole_Type::ff) {
    sumq = sumq + (QN*QN)/sqrt(m_M*m_M + QN*QN);
    sump = sump + ((PN+m_K)*QN)/sqrt(m_M*m_M + (PN+m_K)*(PN+m_K));
  }
  else if (m_dtype == Dipole_Type::fi) {
    sumq = sumq + (Q*Q)/sqrt(m_M*m_M + Q*Q);
    sump = sump + (P*Q)/sqrt(m_M*m_M + P*P);
    // subtract initial state particle (position 0 in old/newdipole)
    N = N-1;
  }
  else {
    prod = 0.;
  }
  msg_Debugging()<<pow(m_u,3.*N-4.)<<" * "<<sumq/sump<<" * "<<prod
                 <<" = "<<pow(m_u,3.*N-4.)*sumq/sump*prod<<std::endl;
  m_weight = pow(m_u,3.*N-4.) * sumq/sump * prod;
}

void Weight_Jacobian_Mapping::CalculateMax() {
  m_maxweight = 1.;
}
