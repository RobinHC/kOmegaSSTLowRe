/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2012 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::incompressible::RASModels::kOmegaSSTLowReLowRe

Description
    Implementation of the k-omega-SST turbulence model for incompressible
    flows with Fluent v15.0 low-Re Extensions

    Turbulence model described in:
    \verbatim
        Menter, F., Esch, T.,
        "Elements of Industrial Heat Transfer Prediction",
        16th Brazilian Congress of Mechanical Engineering (COBEM),
        Nov. 2001.
    \endverbatim

    with the addition of the optional F3 term for rough walls from
    \verbatim
        Hellsten, A.
        "Some Improvements in Menter’s k-omega-SST turbulence model"
        29th AIAA Fluid Dynamics Conference,
        AIAA-98-2554,
        June 1998.
    \endverbatim

    Note that this implementation is written in terms of alpha diffusion
    coefficients rather than the more traditional sigma (alpha = 1/sigma) so
    that the blending can be applied to all coefficuients in a consistent
    manner.  The paper suggests that sigma is blended but this would not be
    consistent with the blending of the k-epsilon and k-omega models.

    Also note that the error in the last term of equation (2) relating to
    sigma has been corrected.

    Wall-functions are applied in this implementation by using equations (14)
    to specify the near-wall omega as appropriate.

    The blending functions (15) and (16) are not currently used because of the
    uncertainty in their origin, range of applicability and that is y+ becomes
    sufficiently small blending u_tau in this manner clearly becomes nonsense.

    The default model coefficients correspond to the following:
    \verbatim
        kOmegaSSTLowReLowReCoeffs
        {
            beta1       0.075;
            beta2       0.0828;
            betaStar    0.09;
            gamma1      0.5532;
            gamma2      0.4403;
            a1          0.31;
            b1          1.0;
            c1          10.0;
            F3          no;
        }
    \endverbatim

SourceFiles
    kOmegaSSTLowReLowRe.C

\*---------------------------------------------------------------------------*/

#ifndef kOmegaSSTLowRe_H
#define kOmegaSSTLowRe_H

#include "RASModel.H"
#include "eddyViscosity.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace RASModels
{

/*---------------------------------------------------------------------------*\
                           Class kOmegaSSTLowRe Declaration
\*---------------------------------------------------------------------------*/

template<class BasicTurbulenceModel>
class kOmegaSSTLowRe
:
    public eddyViscosity<RASModel<BasicTurbulenceModel> >
{
    // Private Member Functions

        // Disallow default bitwise copy construct and assignment
        kOmegaSSTLowRe(const kOmegaSSTLowRe&);
        kOmegaSSTLowRe& operator=(const kOmegaSSTLowRe&);


protected:

    // Protected data

        // Model coefficients
            dimensionedScalar betaInf_;
            dimensionedScalar beta1_;
            dimensionedScalar beta2_;

            dimensionedScalar RBeta_;
            dimensionedScalar RK_;
            dimensionedScalar ROmega_;
            dimensionedScalar betaStarInf_;
            dimensionedScalar alphaStarInf_;
            dimensionedScalar kappa_;
            dimensionedScalar sigmaOmega1_;
            dimensionedScalar sigmaOmega2_;
            dimensionedScalar sigmaK1_;
            dimensionedScalar sigmaK2_;
            dimensionedScalar alphaZero_;

            dimensionedScalar a1_;
            dimensionedScalar b1_;
            dimensionedScalar c1_;

            Switch F3_;

        // Fields

            //- Wall distance
            //  Note: different to wall distance in parent RASModel
            //  which is for near-wall cells only
            const volScalarField& y_;

            volScalarField k_;
            volScalarField omega_;

    // Private Member Functions

        tmp<volScalarField> ReT() const;
        tmp<volScalarField> alphaStar() const;
        tmp<volScalarField> alpha(const volScalarField& F1) const;
        tmp<volScalarField> betaStar() const;

        tmp<volScalarField> F1(const volScalarField& CDkOmega) const;
        tmp<volScalarField> F2() const;
        tmp<volScalarField> F3() const;
        tmp<volScalarField> F23() const;

        tmp<volScalarField> blend
        (
            const volScalarField& F1,
            const dimensionedScalar& psi1,
            const dimensionedScalar& psi2
        ) const
        {
            return F1*(psi1 - psi2) + psi2;
        }

        tmp<volScalarField> alphaInf(const volScalarField& F1) const
        {
            return blend(F1, (beta1_/betaStarInf_ - sqr(kappa_)
                / (sigmaOmega1_ * sqrt(betaStarInf_))),
                (beta2_/betaStarInf_ - sqr(kappa_)/(sigmaOmega2_
                * sqrt(betaStarInf_))));
        }

        tmp<volScalarField> betaI(const volScalarField& F1) const
        {
            return blend(F1, beta1_, beta2_);
        }

        tmp<volScalarField> beta(const volScalarField& F1) const
        {
            return betaI(F1);   //non-compressible version
        }

        tmp<volScalarField> sigmaK(const volScalarField& F1) const
        {
            return 1.0 / blend(F1, 1.0/sigmaK1_, 1.0/sigmaK2_);
        }

        tmp<volScalarField> sigmaOmega(const volScalarField& F1) const
        {
            return 1.0 / blend(F1, 1.0/sigmaOmega1_, 1.0/sigmaOmega2_);
        }

       // void correctNut(const volScalarField& S2);


    // Protected Member Functions

        virtual void correctNut();
        //virtual void correctNut();
        /*virtual tmp<fvScalarMatrix> kSource() const;
        virtual tmp<fvScalarMatrix> omegaSource() const;
        virtual tmp<fvScalarMatrix> Qsas
        (
            const volScalarField& S2,
            const volScalarField& gamma,
            const volScalarField& beta
        ) const;*/


public:

    typedef typename BasicTurbulenceModel::alphaField alphaField;
    typedef typename BasicTurbulenceModel::rhoField rhoField;
    typedef typename BasicTurbulenceModel::transportModel transportModel;


    //- Runtime type information
    TypeName("kOmegaSSTLowRe");


    // Constructors

        //- Construct from components
        kOmegaSSTLowRe
        (
            const alphaField& alpha,
            const rhoField& rho,
            const volVectorField& U,
            const surfaceScalarField& alphaRhoPhi,
            const surfaceScalarField& phi,
            const transportModel& transport,
            const word& propertiesName = turbulenceModel::propertiesName,
            const word& type = typeName
        );


    //- Destructor
    virtual ~kOmegaSSTLowRe()
    {}


    // Member Functions

        //- Re-read model coefficients if they have changed
        virtual bool read();

        //- Return the effective diffusivity for k


        //- Return the effective diffusivity for k
        tmp<volScalarField> DkEff(const volScalarField& F1) const
        {
            return tmp<volScalarField>
            (
                new volScalarField
                (
                    "DkEff", (this->nut_ / sigmaK(F1)) + this->nu()
                )
            );
        }

        //- Return the effective diffusivity for omega
        tmp<volScalarField> DomegaEff(const volScalarField& F1) const
        {
            return tmp<volScalarField>
            (
                new volScalarField
                (
                    "DomegaEff", (this->nut_ / sigmaOmega(F1)) + this->nu()
                )
            );
        }

        //- Return the turbulence kinetic energy
        virtual tmp<volScalarField> k() const
        {
            return k_;
        }

        //- Return the turbulence specific dissipation rate
        virtual tmp<volScalarField> omega() const
        {
            return omega_;
        }

        //- Return the turbulence kinetic energy dissipation rate
        virtual tmp<volScalarField> epsilon() const
        {
            return tmp<volScalarField>
            (
                new volScalarField
                (
                    IOobject
                    (
                        "epsilon",
                        this->mesh_.time().timeName(),
                        this->mesh_
                    ),
                    0.09*k_*omega_,                 //not SST version!!!
                    omega_.boundaryField().types()
                )
            );
        }

        //- Solve the turbulence equations and correct the turbulence viscosity
        virtual void correct();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace RASModels
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
#ifdef NoRepository
#   include "kOmegaSSTLowRe.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
#endif

// ************************************************************************* //
