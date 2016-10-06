/******************************************************************************
*       SOFA, Simulation Open-Framework Architecture, development version     *
*                (c) 2006-2016 INRIA, USTL, UJF, CNRS, MGH                    *
*                                                                             *
* This library is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as published by    *
* the Free Software Foundation; either version 2.1 of the License, or (at     *
* your option) any later version.                                             *
*                                                                             *
* This library is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License *
* for more details.                                                           *
*                                                                             *
* You should have received a copy of the GNU Lesser General Public License    *
* along with this library; if not, write to the Free Software Foundation,     *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.          *
*******************************************************************************
*                               SOFA :: Modules                               *
*                                                                             *
* Authors: The SOFA Team and external contributors (see Authors.txt)          *
*                                                                             *
* Contact information: contact@sofa-framework.org                             *
******************************************************************************/
#ifndef SOFA_COMPONENT_CONSTRAINTSET_UNILATERALINTERACTIONCONSTRAINT_INL
#define SOFA_COMPONENT_CONSTRAINTSET_UNILATERALINTERACTIONCONSTRAINT_INL

#include <SofaConstraint/UnilateralInteractionConstraint.h>
#include <sofa/core/visual/VisualParams.h>
#include <sofa/defaulttype/Vec.h>
#include <sofa/helper/gl/template.h>
namespace sofa
{

namespace component
{

namespace constraintset
{

template<class DataTypes>
void UnilateralInteractionConstraint<DataTypes>::addContact(double mu, Deriv norm, Coord P, Coord Q, Real contactDistance, int m1, int m2, Coord /*Pfree*/, Coord /*Qfree*/, long id, PersistentID localid)
{
    contacts.resize(contacts.size() + 1);
    Contact &c = contacts.back();

    c.P			= P;
    c.Q			= Q;
    c.m1		= m1;
    c.m2		= m2;
    c.norm		= norm;
    c.t			= Deriv(norm.z(), norm.x(), norm.y());
    c.s			= cross(norm, c.t);
    c.s			= c.s / c.s.norm();
    c.t			= cross((-norm), c.s);
    c.mu		= mu;
    c.contactId = id;
    c.localId	= localid;
    c.contactDistance = contactDistance;

    /*
    if (this->f_printLog.getValue())
    {
    	std::cout << "P : " << P << " - PFree : " << Pfree << std::endl;
    	std::cout << "Q : " << Q << " - QFree : " << Qfree << std::endl;
    }
    */
}


template<class DataTypes>
void UnilateralInteractionConstraint<DataTypes>::buildConstraintMatrix(const core::ConstraintParams *, DataMatrixDeriv &c1_d, DataMatrixDeriv &c2_d, unsigned int &contactId
        , const DataVecCoord &, const DataVecCoord &)
{
    assert(this->mstate1);
    assert(this->mstate2);

    if (this->mstate1 == this->mstate2)
    {
        MatrixDeriv& c1 = *c1_d.beginEdit();

        for (unsigned int i = 0; i < contacts.size(); i++)
        {
            Contact& c = contacts[i];

            c.id = contactId++;

            MatrixDerivRowIterator c1_it = c1.writeLine(c.id);

            c1_it.addCol(c.m1, -c.norm);
            c1_it.addCol(c.m2, c.norm);

            if (c.mu > 0.0)
            {
                c1_it = c1.writeLine(c.id + 1);
                c1_it.setCol(c.m1, -c.t);
                c1_it.setCol(c.m2, c.t);

                c1_it = c1.writeLine(c.id + 2);
                c1_it.setCol(c.m1, -c.s);
                c1_it.setCol(c.m2, c.s);

                contactId += 2;
            }
        }

        c1_d.endEdit();
    }
    else
    {
        MatrixDeriv& c1 = *c1_d.beginEdit();
        MatrixDeriv& c2 = *c2_d.beginEdit();

        for (unsigned int i = 0; i < contacts.size(); i++)
        {
            Contact& c = contacts[i];

            c.id = contactId++;

            const Deriv u(1,0,0);

            MatrixDerivRowIterator c1_it = c1.writeLine(c.id);
            c1_it.addCol(c.m1, -c.norm);

            MatrixDerivRowIterator c2_it = c2.writeLine(c.id);
            c2_it.addCol(c.m2, c.norm);

            if (c.mu > 0.0)
            {
                c1_it = c1.writeLine(c.id + 1);
                c1_it.setCol(c.m1, -c.t);

                c1_it = c1.writeLine(c.id + 2);
                c1_it.setCol(c.m1, -c.s);

                c2_it = c2.writeLine(c.id + 1);
                c2_it.setCol(c.m2, c.t);

                c2_it = c2.writeLine(c.id + 2);
                c2_it.setCol(c.m2, c.s);

                contactId += 2;
            }
        }

        c1_d.endEdit();
        c2_d.endEdit();
    }
}


template<class DataTypes>
void UnilateralInteractionConstraint<DataTypes>::getPositionViolation(defaulttype::BaseVector *v)
{
    const VecCoord &PfreeVec = this->getMState2()->read(core::ConstVecCoordId::freePosition())->getValue();
    const VecCoord &QfreeVec = this->getMState1()->read(core::ConstVecCoordId::freePosition())->getValue();

    Real dfree = (Real)0.0;
    Real dfree_t = (Real)0.0;
    Real dfree_s = (Real)0.0;

    const unsigned int cSize = contacts.size();

    for (unsigned int i = 0; i < cSize; i++)
    {
        const Contact& c = contacts[i];

        // Compute dfree, dfree_t and d_free_s

        const Coord &Pfree =  PfreeVec[c.m2];
        const Coord &Qfree =  QfreeVec[c.m1];

        const Coord PPfree = Pfree - c.P;
        const Coord QQfree = Qfree - c.Q;

        const Real ref_dist = PPfree.norm() + QQfree.norm();

        dfree = dot(Pfree - Qfree, c.norm) - c.contactDistance;
        const Real delta = dot(c.P - c.Q, c.norm) - c.contactDistance;

        if ((helper::rabs(delta) < 0.00001 * ref_dist) && (helper::rabs(dfree) < 0.00001 * ref_dist))
        {
            dfree_t = dot(PPfree, c.t) - dot(QQfree, c.t);
            dfree_s = dot(PPfree, c.s) - dot(QQfree, c.s);
        }
        else if (helper::rabs(delta - dfree) > 0.001 * delta)
        {
            const Real dt = delta / (delta - dfree);

            if (dt > 0.0 && dt < 1.0)
            {
                const Coord Pt		= c.P * (1 - dt) + Pfree * dt;
                const Coord Qt		= c.Q * (1 - dt) + Qfree * dt;
                const Coord PtPfree = Pfree - Pt;
                const Coord QtQfree = Qfree - Qt;

                dfree_t = dot(PtPfree, c.t) - dot(QtQfree, c.t);
                dfree_s = dot(PtPfree, c.s) - dot(QtQfree, c.s);
            }
            else if (dfree < 0.0)
            {
                dfree_t = dot(PPfree, c.t) - dot(QQfree, c.t);
                dfree_s = dot(PPfree, c.s) - dot(QQfree, c.s);
            }
            else
            {
                dfree_t = 0;
                dfree_s = 0;
            }
        }
        else
        {
            dfree_t = dot(PPfree, c.t) - dot(QQfree, c.t);
            dfree_s = dot(PPfree, c.s) - dot(QQfree, c.s);
        }

        // Sets dfree in global violation vector

        v->set(c.id, dfree);

        c.dfree = dfree; // PJ : For isActive() method. Don't know if it's still usefull.

        if (c.mu > 0.0)
        {
            v->set(c.id + 1, dfree_t);
            v->set(c.id + 2, dfree_s);
        }
    }
}


template<class DataTypes>
void UnilateralInteractionConstraint<DataTypes>::getVelocityViolation(defaulttype::BaseVector *v)
{
    const VecDeriv &PvfreeVec = this->getMState2()->read(core::ConstVecDerivId::freeVelocity())->getValue();
    const VecDeriv &QvfreeVec = this->getMState1()->read(core::ConstVecDerivId::freeVelocity())->getValue();

    const unsigned int cSize = contacts.size();

    for (unsigned int i = 0; i < cSize; i++)
    {
        const Contact& c = contacts[i];

        const Deriv QP_vfree = PvfreeVec[c.m2] - QvfreeVec[c.m1];

        v->set(c.id, dot(QP_vfree, c.norm)); // dfree

        if (c.mu > 0.0)
        {
            v->set(c.id + 1, dot(QP_vfree, c.t)); // dfree_t
            v->set(c.id + 2, dot(QP_vfree, c.s)); // dfree_s
        }
    }
}


template<class DataTypes>
void UnilateralInteractionConstraint<DataTypes>::getConstraintViolation(const core::ConstraintParams *cparams, defaulttype::BaseVector *v, const DataVecCoord &, const DataVecCoord &
        , const DataVecDeriv &, const DataVecDeriv &)
{
    switch (cparams->constOrder())
    {
    case core::ConstraintParams::POS_AND_VEL :
    case core::ConstraintParams::POS :
        getPositionViolation(v);
        break;

    case core::ConstraintParams::ACC :
    case core::ConstraintParams::VEL :
        getVelocityViolation(v);
        break;

    default :
        serr << "UnilateralInteractionConstraint doesn't implement " << cparams->getName() << " constraint violation\n";
        break;
    }
}


template<class DataTypes>
void UnilateralInteractionConstraint<DataTypes>::getConstraintInfo(const core::ConstraintParams*, VecConstraintBlockInfo& blocks, VecPersistentID& ids, VecConstCoord& /*positions*/, VecConstDeriv& directions, VecConstArea& /*areas*/)
{
    if (contacts.empty()) return;
    const bool friction = (contacts[0].mu > 0.0); /// @todo: can there be both friction-less and friction contacts in the same UnilateralInteractionConstraint ???
    ConstraintBlockInfo info;
    info.parent = this;
    info.const0 = contacts[0].id;
    info.nbLines = friction ? 3 : 1;
    info.hasId = true;
    info.offsetId = ids.size();
    info.hasDirection = true;
    info.offsetDirection = directions.size();
    info.nbGroups = contacts.size();

    for (unsigned int i=0; i<contacts.size(); i++)
    {
        Contact& c = contacts[i];
        ids.push_back( yetIntegrated ? c.contactId : -c.contactId);
        directions.push_back( c.norm );
        if (friction)
        {
            directions.push_back( c.t );
            directions.push_back( c.s );
        }
    }

    yetIntegrated = true;

    blocks.push_back(info);
}

template<class DataTypes>
void UnilateralInteractionConstraint<DataTypes>::getConstraintResolution(const core::ConstraintParams *, std::vector<core::behavior::ConstraintResolution*>& resTab, unsigned int& offset)
{
    if(contactsStatus)
    {
        delete[] contactsStatus;
        contactsStatus = NULL;
    }

    if (contacts.size() > 0)
    {
        contactsStatus = new bool[contacts.size()];
        memset(contactsStatus, 0, sizeof(bool)*contacts.size());
    }

    for(unsigned int i=0; i<contacts.size(); i++)
    {
        Contact& c = contacts[i];
        if(c.mu > 0.0)
        {
//			bool& temp = contactsStatus.at(i);
            UnilateralConstraintResolutionWithFriction* ucrwf = new UnilateralConstraintResolutionWithFriction(c.mu, NULL, &contactsStatus[i]);
            ucrwf->tolerance = customTolerance;
            resTab[offset] = ucrwf;

            // TODO : cette méthode de stockage des forces peu mal fonctionner avec 2 threads quand on utilise l'haptique
//			resTab[offset] = new UnilateralConstraintResolutionWithFriction(c.mu, &prevForces, &contactsStatus[i]);
            offset += 3;
        }
        else
            resTab[offset++] = new UnilateralConstraintResolution();
    }
}

template<class DataTypes>
bool UnilateralInteractionConstraint<DataTypes>::isActive() const
{
    for(unsigned int i = 0; i < contacts.size(); i++)
        if(contacts[i].dfree < 0)
            return true;

    return false;
}

template<class DataTypes>
void UnilateralInteractionConstraint<DataTypes>::draw(const core::visual::VisualParams* vparams)
{

    if (!vparams->displayFlags().getShowInteractionForceFields()) return;

    vparams->drawTool()->saveLastState();
    vparams->drawTool()->setLighting(false);


    std::vector<defaulttype::Vector3> positions;
    std::vector<defaulttype::Vec4f> colors;

    for (unsigned int i=0; i<contacts.size(); i++)
    {
        const Contact& c = contacts[i];
        defaulttype::Vec4f color = defaulttype::Vec4f(1, 0, 0, 1);
        positions.push_back(defaulttype::Vector3(c.P[0], c.P[1], c.P[2]));
        positions.push_back(defaulttype::Vector3(c.Q[0], c.Q[1], c.Q[2]));
        vparams->drawTool()->drawLines(positions, 5.0, defaulttype::Vec4f(1, 0, 0, 1));
        positions.clear();
        
        colors.push_back(defaulttype::Vec4f(1, 1, 1, 1));
        colors.push_back(defaulttype::Vec4f(0, 0.5, 0.5, 1));
        colors.push_back(defaulttype::Vec4f(0, 0, 0, 1));
        colors.push_back(defaulttype::Vec4f(0, 0.5, 0.5, 1));

        positions.push_back(defaulttype::Vector3(c.P[0], c.P[1], c.P[2]));
        positions.push_back(defaulttype::Vector3((c.P+c.norm)[0], (c.P + c.norm)[1], (c.Q + c.norm)[2]));
        positions.push_back(defaulttype::Vector3(c.Q[0], c.Q[1], c.Q[2]));
        positions.push_back(defaulttype::Vector3((c.Q - c.norm)[0], (c.Q - c.norm)[1], (c.Q - c.norm)[2]));

        vparams->drawTool()->drawLines(positions, 3.0, defaulttype::Vec4f(1, 0, 0, 1));

    }
    vparams->drawTool()->restoreLastState();
}

} // namespace constraintset

} // namespace component

} // namespace sofa

#endif
