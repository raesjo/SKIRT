/*//////////////////////////////////////////////////////////////////
////       SKIRT -- an advanced radiative transfer code         ////
////       © Astronomical Observatory, Ghent University         ////
///////////////////////////////////////////////////////////////// */

#include "OligoFitScheme.hpp"

#include "Array.hpp"
#include "FITSInOut.hpp"
#include "FatalError.hpp"
#include "FilePaths.hpp"
#include "Log.hpp"
#include "Optimization.hpp"
#include "ParameterRange.hpp"
#include "ParameterRanges.hpp"
#include "ReferenceImage.hpp"
#include "ReferenceImages.hpp"
#include <QList>

////////////////////////////////////////////////////////////////////

OligoFitScheme::OligoFitScheme()
    : _simulation(0), _ranges(0), _rimages(0), _optim(0)
{
}

////////////////////////////////////////////////////////////////////

void OligoFitScheme::runSelf()
{
    _optim->initialize();
    while(!_optim->done())
    {
        _optim->step();
    }
}

////////////////////////////////////////////////////////////////////

void OligoFitScheme::setSimulation(AdjustableSkirtSimulation* value)
{
    if (_simulation) delete _simulation;
    _simulation = value;
    if (_simulation) _simulation->setParent(this);
}

////////////////////////////////////////////////////////////////////

AdjustableSkirtSimulation* OligoFitScheme::simulation() const
{
    return _simulation;
}

////////////////////////////////////////////////////////////////////

void OligoFitScheme::setParameterRanges(ParameterRanges* value)
{
    if (_ranges) delete _ranges;
    _ranges = value;
    if (_ranges) _ranges->setParent(this);
}

////////////////////////////////////////////////////////////////////

ParameterRanges* OligoFitScheme::parameterRanges() const
{
    return _ranges;
}

////////////////////////////////////////////////////////////////////

void OligoFitScheme::setReferenceImages(ReferenceImages *value)
{
    if (_rimages) delete _rimages;
    _rimages = value;
    if (_rimages) _rimages->setParent(this);
}

////////////////////////////////////////////////////////////////////

ReferenceImages* OligoFitScheme::referenceImages() const
{
    return _rimages;
}

////////////////////////////////////////////////////////////////////

void OligoFitScheme::setOptim(Optimization *value)
{
    if (_optim) delete _optim;
    _optim = value;
    if (_optim) _optim->setParent(this);
}

////////////////////////////////////////////////////////////////////

Optimization* OligoFitScheme::optim() const
{
    return _optim;
}

////////////////////////////////////////////////////////////////////

double OligoFitScheme::objective(AdjustableSkirtSimulation::ReplacementDict replacement,
                                 QList<double> *DiskLuminosities, QList<double> *BulgeRatios, QList<double> *Chis,
                                 int index)
{
    //perform the adjusted simulation
    QString prefix = "tmp/tmp_"+QString::number(index);
    _simulation->performWith(replacement, prefix);
    QString instrname = find<AdjustableSkirtSimulation>()->instrname();

    //read the simulation frames and compare the frame size with the reference image
    int counter=0;
    QList<Array> Simulations;
    QList<QList<Array>> frames;
    QList<QList<double>> luminosities;

    foreach (ReferenceImage* rima, _rimages->images())
    {
        int nx,ny,nz; //dummy values;
        Array diskTotal, bulgeTotal;

        QString filepath = _paths->output(prefix+"_"+instrname+"_stellar_0_"+QString::number(counter)+".fits");
        FITSInOut::read(filepath,diskTotal,nx,ny,nz);

        filepath = _paths->output(prefix+"_"+instrname+"_stellar_1_"+QString::number(counter)+".fits");
        FITSInOut::read(filepath,bulgeTotal,nx,ny,nz);

        int framesize = (rima->xdim())*(rima->ydim());
        int disksize = diskTotal.size();
        int bulgesize = bulgeTotal.size();
        if (framesize != disksize && framesize != bulgesize)
            throw FATALERROR("Simulations and Reference Images have different dimensions");

        Simulations.append(diskTotal);
        Simulations.append(bulgeTotal);
        frames.append(Simulations);
        Simulations.clear();
        luminosities.append(*(DiskLuminosities));
        luminosities.append(*(BulgeRatios));
        DiskLuminosities->clear();
        BulgeRatios->clear();
        counter++;
    }

    //determine the best fitting luminosities and lowest chi2 value
    double test_chi2functions = _rimages->chi2(&frames,&luminosities, Chis);
    return test_chi2functions;
}

////////////////////////////////////////////////////////////////////
