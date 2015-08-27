/*//////////////////////////////////////////////////////////////////
////       SKIRT -- an advanced radiative transfer code         ////
////       © Astronomical Observatory, Ghent University         ////
///////////////////////////////////////////////////////////////// */

#include <cmath>
#include "Box.hpp"
#include "FatalError.hpp"
#include "Log.hpp"
#include "NR.hpp"
#include "ParallelFactory.hpp"
#include "PeerToPeerCommunicator.hpp"
#include "Position.hpp"
#include "Random.hpp"

using namespace std;

//////////////////////////////////////////////////////////////////////

Random::Random()
    : _seed(4357), _parfac(0)
{
}

//////////////////////////////////////////////////////////////////////

void Random::setupSelfBefore()
{
    SimulationItem::setupSelfBefore();

    _parfac = find<ParallelFactory>();
    int Nthreads = _parfac->maxThreadCount();
    _mtv.resize(Nthreads);
    _mtiv.resize(Nthreads);

    initialize(Nthreads);
}

//////////////////////////////////////////////////////////////////////

void Random::initialize(int Nthreads)
{
    unsigned long seed = _seed;
    for (int thread=0; thread<Nthreads; thread++)
    {
        find<Log>()->info("Initializing random number generator for thread number "
                          + QString::number(thread) + " with seed " + QString::number(seed) + "... ");
        vector<unsigned long>& mt = _mtv[thread];
        int& mti = _mtiv[thread];
        mt.resize(624);
        mt[0] = seed & 0xffffffff;
        for (mti=1; mti<624; mti++)
            mt[mti] = (69069 * mt[mti-1]) & 0xffffffff;
        ++seed;
    }
}

//////////////////////////////////////////////////////////////////////

void Random::setSeed(int seed)
{
    _seed = seed;
}

//////////////////////////////////////////////////////////////////////

int Random::seed() const
{
    return _seed;
}

//////////////////////////////////////////////////////////////////////

void Random::randomize()
{
    PeerToPeerCommunicator* comm = find<PeerToPeerCommunicator>();

    int Nthreads = _parfac->maxThreadCount();
    _mtv.resize(Nthreads);      // Because the number of threads can be different during and after the setup
    _mtiv.resize(Nthreads);     // of the simulation.

    _seed = _seed + Nthreads * comm->rank();

    initialize(Nthreads);
}

//////////////////////////////////////////////////////////////////////

double
Random::uniform()
{
    int thread = _parfac->currentThreadIndex();
    vector<unsigned long>& mt = _mtv[thread];
    int& mti = _mtiv[thread];
    double ans = 0.0;
    do
    {
        unsigned long y;
        static unsigned long mag01[2]={0x0,0x9908b0df};
        if (mti >= 624)
        {
            int kk;
            for (kk=0;kk<227;kk++)
            {
                y = (mt[kk]&0x80000000)|(mt[kk+1]&0x7fffffff);
                mt[kk] = mt[kk+397] ^ (y >> 1) ^ mag01[y & 0x1];
            }
            for (;kk<624-1;kk++)
            {
                y = (mt[kk]&0x80000000)|(mt[kk+1]&0x7fffffff);
                mt[kk] = mt[kk-227] ^ (y >> 1) ^ mag01[y & 0x1];
            }
            y = (mt[623]&0x80000000)|(mt[0]&0x7fffffff);
            mt[623] = mt[396] ^ (y >> 1) ^ mag01[y & 0x1];
            mti = 0;
        }
        y = mt[mti++];
        y ^= (y>>11);
        y ^= (y<<7) & 0x9d2c5680;
        y ^= (y<<15) & 0xefc60000;
        y ^= (y>>18);
        ans = static_cast<double>(y) / static_cast<unsigned long>(0xffffffff);
    }
    while (ans<=0.0 || ans>=1.0);
    return ans;
}

//////////////////////////////////////////////////////////////////////

double
Random::cdf(const Array& xv, const Array& Xv)
{
    double X = uniform();
    int i = NR::locate_clip(Xv, X);
    return NR::interpolate_linlin(X, Xv[i], Xv[i+1], xv[i], xv[i+1]);
}

//////////////////////////////////////////////////////////////////////

double
Random::gauss()
{
    double rsq, v1, v2;
    do {
        v1 = 2.0*uniform()-1.0;
        v2 = 2.0*uniform()-1.0;
        rsq = v1*v1+v2*v2;
    } while (rsq>=1.0 || rsq==0.0);
    return v2*sqrt(-2.0*log(rsq)/rsq);
}

//////////////////////////////////////////////////////////////////////

double
Random::expon()
{
    return -log(1.0-uniform());
}

//////////////////////////////////////////////////////////////////////

double
Random::exponcutoff(double xmax)
{
    if (xmax==0.0)
        return 0.0;
    else if (xmax<1e-10)
        return uniform()*xmax;
    double x = -log(1.0-uniform()*(1.0-exp(-xmax)));
    while (x>xmax)
    {
        x = -log(1.0-uniform()*(1.0-exp(-xmax)));
    }
    return x;
}

//////////////////////////////////////////////////////////////////////

Direction
Random::direction()
{
    double theta = acos(2.0*uniform()-1.0);
    double phi = 2.0*M_PI*uniform();
    return Direction(theta,phi);
}

//////////////////////////////////////////////////////////////////////

Direction
Random::cosdirection()
{
    double X = uniform();
    double theta = 0.0;
    if (X<0.5)
        theta = acos(sqrt(1.0-2.0*X));
    else
        theta = acos(-sqrt(2.0*X-1.0));
    double phi = 2.0*M_PI*uniform();
    return Direction(theta,phi);
}

//////////////////////////////////////////////////////////////////////

Position Random::position(const Box& box)
{
    // generate the random numbers in separate statements to guarantee evaluation order
    // (function arguments are evaluated in different order depending on the compiler)
    double x = uniform();
    double y = uniform();
    double z = uniform();
    return Position(box.fracpos(x,y,z));
}

//////////////////////////////////////////////////////////////////////

double
Random::scpf(double p)
{
    double random;
    if (p==0.0){
        random = asin( sqrt( uniform() )  );
    }
    else{
        double X = uniform();
        double A =( sqrt(p*p*p* (4.0 + p*(3.0+p)*(3.0+p)*(1.0-2.0*X)*(1.0-2.0*X)) ) + (p*p*(3.0+p-2.0*(3.0+p)*X) ) );
        double numerator = pow(2.0,2.0/3.0) * p * pow(A,1.0/3.0);
        double denominator = -2.0 * p  + pow(2.0,1.0/3.0) * pow(A,2.0/3.0);
        //we divinde denominator by numerator here, because we want the arcsec of numerator/denominator and arccos(1/x) = arcsec(x)
        random = acos(denominator/numerator);
    }

    return random;
}

//////////////////////////////////////////////////////////////////////
