#include <faunus/histogram.h>
#include <faunus/io.h>

namespace Faunus {
  //! \param res x value resolution
  //! \param min minimum x value
  //! \param max maximum x value
  histogram::histogram(float res, float min, float max)
    : xytable<float,unsigned long int>(res,min,max) {
      cnt=0;
      xmaxi=max;
      xmini=min;
    }

  //! Increment bin for x value
  void histogram::add(float x) {
    if (x>=xmaxi || x<=xmini) return;
    (*this)(x)++;
    cnt++;
  }

  //! Get bin for x value
  //! \return \f$ \frac{N(r)}{N_{tot}}\f$
  float histogram::get(float x) { return (*this)(x)/float(cnt); }

  //! Show results for all x
  void histogram::write(string file) {
    float g;
    std::ofstream f(file.c_str());
    if (f) {
      f.precision(6);
      for (double x=xmin; x<xmax(); x+=xres) {
        g=get(x);
        if (g!=0.0)
          f << x << " " << g << "\n";
      }
      f.close();
    }
  }

  /*!
   * \param species1 Particle type 1
   * \param species2 Particle type 2
   * \param resolution Histogram resolution (binwidth)
   * \param xmaximum Maximum x value (used for better memory utilisation)
   */
  FAUrdf::FAUrdf(short species1, short species2, float resolution, float xmaximum) :
    histogram(resolution, 0, xmaximum)
  {
    a=species1;
    b=species2;
  }
  FAUrdf::FAUrdf(float resolution, float xmaximum) : histogram(resolution, 0, xmaximum) {};

  /*!
   * Update histogram between two known points
   * \note Uses the container distance function
   */
  void FAUrdf::update(container &c, point &a, point &b) {
    add( c.dist(a, b) );
  }

  /*!
   * Calculate all distances between between species 1
   * and 2 and update the histogram.
   *
   * \note Uses the container function to calculate distances
   */
  void FAUrdf::update(container &c) {
    int n=c.p.size();
    npart=0;
#pragma omp for
    for (int i=0; i<n-1; i++)
      for (int j=i+1; j<n; j++) 
        if ( (c.p[i].id==a && c.p[j].id==b)
            || (c.p[j].id==a && c.p[i].id==b) ) {
          npart++;
          update( c, c.p[i], c.p[j] );
        }
  }

  void FAUrdf::update(container &c, group &g) {
    int n=g.end+1;
    npart=0;
    //#pragma omp for
    for (int i=g.beg; i<n-1; i++) {
      for (int j=i+1; j<n; j++) { 
        if ( (c.p[i].id==a && c.p[j].id==b)
            || (c.p[j].id==a && c.p[i].id==b) ) {
          npart++;
          add( c.dist(c.p[i], c.p[j]) );
        }
      }
    }
  }

  void FAUrdf::update(container &c, point &p, string name) {
    int id=c.atom[name].id, n=c.p.size();
    npart=0;
    //#pragma omp for
    for (int i=0; i<n; ++i)
      if ( c.p[i].id==id ) {
        npart++;
        add( c.dist(p, c.p[i] ));
      }
  }

  /*!
   * Calculate all distances between between species 1
   * and 2 and update the histogram.
   *
   * \warning This function uses a simple distance function (no min. image)
   */
  void FAUrdf::update(vector<particle> &p)
  {
    int n=p.size();
    npart=0;
    //#pragma omp for
    for (int i=0; i<n-1; i++)
      for (int j=i+1; j<n; j++) 
        if ( (p[i].id==a && p[j].id==b)
            || (p[j].id==a && p[i].id==b) ) {
          npart++;
          add( p[i].dist(p[j]) );
        }
  }

  /*!
   * Calculate shell volume at x
   */
  float FAUrdf::volume(float x) { return 4./3.*acos(-1.)*( pow(x+0.5*xres,3)-pow(x-0.5*xres,3) ); }
  /*!
   *  Get g(x) from histogram according to
   *    \f$ g(x) = \frac{N(r)}{N_{tot}} \frac{ 3 } { 4\pi\left [ (x+xres)^3 - x^3 \right ] }\f$
   */
  float FAUrdf::get(float x) {
    return (*this)(x)/(cnt*volume(x)) * 1660.57;
  }

  /*!
   *  Get g(x) from histogram according to
   *    \f$ g(x) = \frac{N(r)}{N_{tot}} \frac{ 3 } { 4\pi\left [ (x+xres)^3 - x^3 \right ] }\f$
   */

  profile::profile(float min, float max, float res) :
    xytable<float,unsigned long int>(res,min,max) { cnt=0; }
  float profile::conc(float x) { return ((*this)(x)>0) ? (*this)(x)/(cnt*volume(x)) : 0; }
  void profile::update(vector<particle> &p) { for (int i=0; i<p.size(); i++) add(p[i]); }
  bool profile::write(string name) {
    io fio;
    std::ostringstream o;
    for (float d=xmin; d<xmax(); d+=xres)
      o << d << " " << conc(d) << endl;
    return fio.writefile(name,o.str());
  }

  float cummsum::volume(double z) { return 1.; }

  cummsum::cummsum(
      particle::type type, particle &center, float max,float res) :
    profile(0,max,res) {
      id=type;
      origo=&center;
    }

  void cummsum::add(particle &p) { if (p.id==id) (*this)(origo->dist(p))++; }

  float cylindric_profile::volume(double z) { return xres*acos(-1.)*r*r; }

  cylindric_profile::cylindric_profile(
      float radius, particle::type type, float min,float max,float res) :
    profile(min,max,res) {
      r=radius;
      id=type;
    }

  void cylindric_profile::add(particle &p) {
    if (p.id==id)
      if (p.z>=xmin && p.z<=xmax())
        if (p.x*p.x+p.y*p.y<=r*r) {
          (*this)(p.z)++;
          cnt++;
        }
  }
}//namespace