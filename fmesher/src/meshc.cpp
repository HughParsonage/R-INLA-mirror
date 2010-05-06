#include <cstddef>
#include <cstring>
#include <set>
#include <map>
#include <sstream>
#include <cmath>

#include "predicates.h"
#include "meshc.h"

#define WHEREAMI __FILE__ << "(" << __LINE__ << ")\t"

namespace fmesh {

  bool MCQ::found(const Dart& d) const
  {
    return (darts_.find(d) != darts_.end());
  }

  bool MCQ::foundQ(const Dart& d) const
  {
    map_type::const_iterator i = darts_.find(d);
    if (i == darts_.end())
      return false;
    return (darts_quality_.find(MCQdv(i->first,i->second)) !=
	    darts_quality_.end());
  }

  const double MCQ::quality(const Dart& d) const
  {
    if (empty())
      return 0.0;
    return darts_.find(d)->second;
  }

  Dart MCQ::quality() const
  {
    if (emptyQ())
      return Dart();
    return darts_quality_.rbegin()->d_;
  }

  void MCQ::insert(const Dart& d)
  {
    double quality_ = calcQ(d);
    if (quality_>0.0) {
      darts_.insert(map_key_type(d,quality_));
      darts_quality_.insert(MCQdv(d,quality_));
    } else if (!only_quality_)
      darts_.insert(map_key_type(d,quality_));
  }

  void MCQ::erase(const Dart& d)
  {
    double quality_;
    map_type::iterator i = darts_.find(d);
    if (i != darts_.end()) {
      quality_ = i->second;
      darts_.erase(i);
      set_type::iterator j = darts_quality_.find(MCQdv(d,quality_));
      if (j != darts_quality_.end())
	darts_quality_.erase(j);
    }
  }







  MCQtri::MCQtri(MeshC* MC, bool only_quality, double quality_limit)
    : MCQ(MC, only_quality), quality_limit_(quality_limit)
  {
    setQ(quality_limit);
  }

  void MCQtri::setQ(double quality_limit)
  {
    quality_limit_ = quality_limit;
  }

  double MCQtri::calcQ(const Dart& d) const {
    double quality_lim_ = quality_limit_; /* TODO: min_vi ql[t][vi] */
    return (calcQtri(d) - quality_lim_);
  };

  double MCQskinny::calcQtri(const Dart& d) const
  {
    double quality_ = MC_->skinnyQuality(d.t());
    return quality_;
  }
  
  double MCQbig::calcQtri(const Dart& d) const
  {
    double quality_ = MC_->bigQuality(d.t());
    return quality_;
  }
  
  double MCQsegm::calcQ(const Dart& d) const
  {
    double quality_ = MC_->encroachedQuality(d);
    Dart dh(d);
    dh.orbit1();
    if (d.t() != dh.t()) {
      double quality1_ = MC_->encroachedQuality(dh);
      if (quality1_>quality_)
	quality_ = quality1_;
    }
    return (quality_-encroached_limit_);
  }

  bool MCQsegm::segm(const Dart& d) const
  {
    if (found(d)) return true;
    Dart dh(d);
    dh.orbit1();
    return ((dh.t() != d.t()) && found(dh));
  }

  void MCQsegm::update(const Dart& d)
  {
    if (found(d)){
      erase(d);
      insert(d);
    }
    Dart dh(d);
    dh.orbit1();
    if ((dh.t() != d.t()) && found(dh)) {
      erase(dh);
      insert(dh);
    }
  }

  double MCQswapable::calcQ(const Dart& d) const
  {
    return (d.isSwapable() ? 1.0 : -1.0);
  }

  double MCQswapableD::calcQ(const Dart& d) const
  {
    return (d.isSwapableD() ? 1.0 : -1.0);
  }

  bool MCQswapable::found(const Dart& d) const
  {
    if (MCQ::found(d)) return true;
    Dart dh(d);
    dh.orbit1();
    if (dh.t() != d.t())
      if (MCQ::found(dh)) return true;
    return false;
  }

  bool MCQswapable::foundQ(const Dart& d) const
  {
    if (MCQ::foundQ(d)) return true;
    Dart dh(d);
    dh.orbit1();
    if (dh.t() != d.t())
      if (MCQ::foundQ(dh)) return true;
    return false;
  }

  const double MCQswapable::quality(const Dart& d) const
  {
    if (MCQ::foundQ(d)) return MCQ::quality(d);
    Dart dh(d);
    dh.orbit1();
    if (dh.t() != d.t())
      return MCQ::quality(d);
    return 0.0;
  }

  void MCQswapable::insert(const Dart& d)
  {
    if (found(d)) return;  /* Don't add duplicates. */
      MCQ::insert(d);
  }

  void MCQswapable::erase(const Dart& d)
  {
    MCQ::erase(d);
    Dart dh(d);
    dh.orbit1();
    if (dh.t() != d.t()) {
      MCQ::erase(dh);
    }
  }

  bool MCQswapable::swapable(const Dart& d) const
  {
    if (foundQ(d)) return true;
    Dart dh(d);
    dh.orbit1();
    /* Boundary edges should never be in the set, but check
       for safety, and then check if found. */
    return ((dh.t() != d.t()) && foundQ(dh));
  }



  double MeshC::encroachedQuality(const Dart& d) const
  /* >0 --> encroached */
  {
    int t(d.t());
    if ((t<0) || (t>=(int)M_->nT())) return -1.0;

    Dart dh(d);
    dh.orbit2rev();
    
    double encr = M_->edgeEncroached(d,M_->S(dh.v()));

    dh.orbit2rev();
    std::cout << WHEREAMI << "encroachedQ("
	      << d.v() << "," << dh.v()
	      << ") = " << encr << std::endl;

    return encr;
  }

  double MeshC::skinnyQuality(int t) const
  {
    if ((t<0) || (t>=(int)M_->nT())) return 0.0;

    double skinny = (M_->triangleCircumcircleRadius(t) / 
		     M_->triangleShortestEdge(t));

    //    std::cout << WHEREAMI << "skinnyQ(" << t << ") = " << skinny << std::endl;

    return skinny;
  }

  double MeshC::bigQuality(int t) const
  {
    return M_->triangleLongestEdge(t);
    //return M_->triangleArea(t);
  }





  /*! Alg 4.3 */
  bool MeshC::recSwapDelaunay(const Dart& d0)
  {
    Dart d1, d2;

    std::cout << WHEREAMI << "Trying to swap " << d0 << std::endl;

    if (d0.isnull() or d0.onBoundary()) {
      std::cout << WHEREAMI << "Not allowed to swap, boundary" << std::endl;
      return true; /* OK. Not allowed to swap. */
    }
    if (isSegment(d0)) {
      std::cout << WHEREAMI << "Not allowed to swap, segment" << std::endl;
      return true ; /* OK. Not allowed to swap. */
    }
    if (d0.circumcircleOK()) {
      Dart dh(d0);
      dh.orbit0rev().orbit2();
      int v = dh.v();
      double result0(d0.inCircumcircle(M_->S(v)));
      dh = d0;
      dh.orbit2rev();
      v = dh.v();
      dh.orbit2();
      dh.orbit1();
      double result1(dh.inCircumcircle(M_->S(v)));
      std::cout << WHEREAMI << "No need to swap, circumcircle OK, ("
		<< result0 << "," << result1 << ")"
		<< std::endl;
      return true; /* OK. Need not swap. */
    }

    std::cout << WHEREAMI << "Swap " << d0 << std::endl;

    /* Get opposing darts. */
    d1 = d0;
    d1.alpha1();
    if (d1.onBoundary()) d1 = Dart(); else d1.alpha2();
    d2 = d0;
    d2.orbit2rev().alpha1(); 
    if (d2.onBoundary()) d2 = Dart(); else d2.alpha2();
    
    //    std::cout << WHEREAMI << "TVpre  = " << std::endl << M_->TVO();
    swapEdge(d0);
    //    std::cout << WHEREAMI << "TVpost = " << std::endl << M_->TVO();
    //    std::cout << WHEREAMI << "TTpost = " << std::endl << M_->TTO();

    if (!d1.isnull()) recSwapDelaunay(d1);
    if (!d2.isnull()) recSwapDelaunay(d2);
    return true;
  }


  /*! Alg 9.3 */
  Dart MeshC::splitTriangleDelaunay(const Dart& td, int v)
  {
    Dart d, d0, d1, d2;

    if (td.isnull()) { return Dart(); }; /* ERROR */
    /* Get opposing darts. */
    d = td;
    if (d.onBoundary()) d0 = Dart(); else {d0 = d; d0.orbit1();} 
    d.orbit2();
    if (d.onBoundary()) d1 = Dart(); else {d1 = d; d1.orbit1();} 
    d.orbit2();
    if (d.onBoundary()) d2 = Dart(); else {d2 = d; d2.orbit1();} 

    //    std::cout << WHEREAMI << "TV = " << std::endl << M_->TVO();
    std::cout << WHEREAMI << "Split triangle " << td << " with vertex " << v << std::endl;
    d = splitTriangle(td,v);
    
    if (!d0.isnull()) recSwapDelaunay(d0);
    if (!d1.isnull()) recSwapDelaunay(d1);
    if (!d2.isnull()) recSwapDelaunay(d2);

    //    std::cout << WHEREAMI << "TV = " << std::endl << M_->TVO();
    
    return d;
  }

  /*! Modified Alg 9.3 */
  Dart MeshC::splitEdgeDelaunay(const Dart& ed, int v)
  {
    Dart d, d0, d1, d2, d3;

    if (ed.isnull()) { return Dart(); }; /* ERROR */
    /* Get opposing darts. */
    d = ed;
    d.orbit2();
    if (d.onBoundary()) d0 = Dart(); else {d0 = d; d0.orbit1();} 
    d.orbit2();
    if (d.onBoundary()) d1 = Dart(); else {d1 = d; d1.orbit1();} 
    d = ed;
    if (d.onBoundary()) {
      d2 = Dart();
      d3 = Dart();
    } else {
      d.orbit0rev();
      if (d.onBoundary()) d2 = Dart(); else {d2 = d; d2.orbit1();} 
      d.orbit2();
      if (d.onBoundary()) d3 = Dart(); else {d3 = d; d3.orbit1();} 
    }

    //    std::cout << WHEREAMI << "TV = " << std::endl << M_->TVO();
    std::cout << WHEREAMI << "Split edge " << ed << " with vertex " << v << std::endl;
    d = splitEdge(ed,v);
    
    if (!d0.isnull()) recSwapDelaunay(d0);
    if (!d1.isnull()) recSwapDelaunay(d1);
    if (!d2.isnull()) recSwapDelaunay(d2);
    if (!d3.isnull()) recSwapDelaunay(d3);

    //    std::cout << WHEREAMI << "TV = " << std::endl << M_->TVO();
    
    return d;
  }

  Dart MeshC::bisectEdgeDelaunay(const Dart& d)
  {
    Dart dh(d);
    int v0(dh.v());
    dh.alpha0();
    int v1(dh.v());

    double s[3];
    Vec::sum(s,M_->S(v0),M_->S(v1));
    Vec::rescale(s,0.5);

    switch (M_->type()) {
    case Mesh::Mtype_manifold:
      /* Fall through to Mtype_plane behaviour; we have no
	 manifold-specific algorithm. */
    case Mesh::Mtype_plane:
      /* Nothing to do! */
      break;
    case Mesh::Mtype_sphere:
      Vec::rescale(s,1./Vec::length(s));
      break;
    }

    return splitEdgeDelaunay(d,addVertices(&s,1));
  }



  void MeshC::calcSteinerPoint(const Dart& d, Point& c)
  {
    M_->triangleCircumcenter(d.t(),c);
    std::cout << WHEREAMI << "Steiner point: ("
	      << c[0] << ","
	      << c[1] << ","
	      << c[2] << ")" << std::endl;
  }



  /*! Alg 9.3 */
  Dart MeshC::insertNode(int v, const Dart& ed)
  {
    Dart td;

    std::cout << WHEREAMI << "Locating node " << v
	      << " " << M_->S(v) << std::endl;

    td = M_->locatePoint(ed,M_->S(v));
    if (td.isnull()) { return Dart(); }; /* ERROR, not found! */
    td = Dart(*M_,td.t());
    Point bary;
    M_->barycentric(td,M_->S(v),bary);
    size_t pattern(size_t(bary[0]>MESH_EPSILON)*1+
		   size_t(bary[1]>MESH_EPSILON)*2+
		   size_t(bary[2]>MESH_EPSILON)*4);
    std::cout << WHEREAMI << "Triangle dart " << td
	      << "\n\t S[t]=("
	      << M_->S(M_->TV(td.t())[0]) << ",\n\t       "
	      << M_->S(M_->TV(td.t())[1]) << ",\n\t       "
	      << M_->S(M_->TV(td.t())[2]) << ")"
	      << "\n\t bary=" << bary
	      << "\n\t pattern=" << pattern
	      << "\n\t S[v]=" << M_->S(v)
	      << std::endl;
    switch (pattern) {
    case 7: // +++
      return splitTriangleDelaunay(td,v);
      break;
    case 6: // -++ Split e0
      td.orbit2();
      std::cout << WHEREAMI << "Edge dart " << td << std::endl;
      return splitEdgeDelaunay(td,v);
      break;
    case 5: // +-+ Split e1
      td.orbit2rev();
      std::cout << WHEREAMI << "Edge dart " << td << std::endl;
      return splitEdgeDelaunay(td,v);
      break;
    case 3: // ++- Split e2
      std::cout << WHEREAMI << "Edge dart " << td << std::endl;
      return splitEdgeDelaunay(td,v);
      break;
    case 1: // +-- Close to node 0, not allowed
    case 2: // -+- Close to node 1, not allowed
    case 4: // --+ Close to node 2, not allowed
      std::cout << WHEREAMI << "ERROR: Attempt to add a duplicate point in triangle " << td << std::endl;
      break;
    case 0: // --- Close to all nodes, should not happen!
      break;
    }

    return Dart();
  }

  bool MeshC::DT(const vertexListT& v_set)
  {
    if (is_pruned_) 
      return false; /* ERROR, cannot safely insert nodes into a pruned
		       triangulation. Call insertNode directly if known to
		       be visible/reachable from a given edge.  */

    if (state_ < State_CHT)
      return false; /* TODO: Add convex enclosure? */

    if (state_ < State_DT)
      if (!prepareDT()) /* Make sure we have a DT. */
	return false;

    int v;
    vertexListT::const_iterator v_iter;
    Dart dh;

    dh = Dart();
    for (v_iter = v_set.begin(); v_iter != v_set.end(); v_iter++) {
      v = *v_iter;
      if (dh.isnull()) dh = Dart(*M_,0);
      dh = insertNode(v,dh); /* Start looking where the previous
				point was found. */
    }
      
    M_->redrawX11("DT finished");

    return true;
  }


  bool MeshC::prepareDT()
  {
    if (state_<State_DT) {
      /* We need to build a DT first. */
      triangleSetT t_set;
      for (int t=0;t<(int)M_->nT();t++)
	t_set.insert(t);
      if (LOP(t_set))
	state_ = State_DT;
    }
    return (state_>=State_DT);
  }


  bool MeshC::prepareCDT()
  {
    if (!prepareDT()) return false; /* Make sure we have a DT. */
    if (state_>=State_CDT)
      return true; /* Nothing to do. Data structures already active. */

    const int* tt;
    int vi;
    Dart d;
    for (int t=0;t<(int)M_->nT();t++) {
      tt = M_->TT()[t];
      for (vi=0;vi<3;vi++)
	if (tt[vi]<0) {
	  d = Dart(*M_,t,1,(vi+1)%3);
	  boundary_.insert(d);
	}
    }

    state_ = State_CDT;
    return true;
  }

  bool MeshC::prepareRCDT(double skinny_limit, double big_limit)
  {
    if (!prepareCDT()) return false; /* Make sure we have a CDT. */

    skinny_.clear();
    big_.clear();
    skinny_.setQ(skinny_limit);
    big_.setQ(big_limit);

    for (int t=0;t<(int)M_->nT();t++) {
      skinny_.insert(Dart(*M_,t));
      big_.insert(Dart(*M_,t));
    }

    state_ = State_RCDT;
    return true;
  }



  bool MeshC::CDTBoundary(const constrListT& constr)
  {
    if (!prepareCDT()) return false;

    constr_boundary_ = constrListT(constr.begin(),constr.end());
    
    return buildCDT();
  };

  bool MeshC::CDTInterior(const constrListT& constr)
  {
    if (!prepareCDT()) return false;

    constr_interior_ = constrListT(constr.begin(),constr.end());
    
    return buildCDT();
  };

  bool MeshC::CDT(const constrListT& boundary, const constrListT& interior)
  {
    if (!prepareCDT()) return false;

    constr_boundary_ = constrListT(boundary.begin(),boundary.end());
    constr_interior_ = constrListT(interior.begin(),interior.end());
    
    return buildCDT();
  };






  bool MeshC::LOP(MCQswapableD& swapable)
  {
    std::cout << WHEREAMI << "LOP swapable: "
	      << swapable.countQ() << "/" << swapable.count() << std::endl;
    /* Swap edges, until none are swapable. */
    while (!swapable.emptyQ()) {
      swapEdge(swapable.beginQ()->d_,swapable);
      std::cout << WHEREAMI << "LOP swapable: "
		<< swapable.countQ() << "/" << swapable.count() << std::endl;
    }

    //    M_->redrawX11("LOP finished");

    return true;
  }

  bool MeshC::LOP(const triangleSetT& t_set)
  {
    /* Locate interior edges */
    Dart dh, dh2;
    MCQswapableD swapable(this);
    for (triangleSetT::const_iterator ci=t_set.begin();
	 ci != t_set.end(); ci++) {
      dh = Dart(*M_,(*ci));
      for (int vi=0; vi<3; vi++) {
	dh2 = dh;
	dh2.orbit1();
	if ((dh.t() != dh2.t()) /* Only add if not on boundary */
	    && (t_set.find(dh2.t()) != t_set.end())
	    /* Only add if the neighbouring triangle is also in the set. */
	    && ((state_<State_CDT)
		|| ((!boundary_.segm(dh))
		    && (!interior_.segm(dh))))) /* Don't add CDT segments. */
	  swapable.insert(dh); /* MCQswapableD takes care of duplicates. */
	dh.orbit2();
      }
    }

    return LOP(swapable);
  }



  typedef std::list<IntPair> BoundaryList;
  
  void prevnext(BoundaryList::iterator& prev,
		BoundaryList::iterator& curr,
		BoundaryList::iterator& next)
  {
    for (curr--; curr->second == 0; curr--) {};
    for (curr++; curr->second == 0; curr++) {};
    next = curr;
    for (next++; next->second == 0; next++) {};
    prev = curr;
    for (prev--; prev->second == 0; prev--) {};
  }

  void prev(BoundaryList::iterator& prev,
	    BoundaryList::iterator& curr,
	    BoundaryList::iterator& next)
  {
    for (curr--; curr->second == 0; curr--) {};
    next = curr;
    for (next++; next->second == 0; next++) {};
    prev = curr;
    for (prev--; prev->second == 0; prev--) {};
  }

  void next(BoundaryList::iterator& prev,
	    BoundaryList::iterator& curr,
	    BoundaryList::iterator& next)
  {
    for (curr++; curr->second == 0; curr++) {};
    next = curr;
    for (next++; next->second == 0; next++) {};
    prev = curr;
    for (prev--; prev->second == 0; prev--) {};
  }

  void prev(BoundaryList::reverse_iterator& prev,
	    BoundaryList::reverse_iterator& curr,
	    BoundaryList::reverse_iterator& next)
  {
    for (curr--; curr->second == 0; curr--) {};
    next = curr;
    for (next++; next->second == 0; next++) {};
    prev = curr;
    for (prev--; prev->second == 0; prev--) {};
  }

  void next(BoundaryList::reverse_iterator& prev,
	    BoundaryList::reverse_iterator& curr,
	    BoundaryList::reverse_iterator& next)
  {
    for (curr++; curr->second == 0; curr++) {};
    next = curr;
    for (next++; next->second == 0; next++) {};
    prev = curr;
    for (prev--; prev->second == 0; prev--) {};
  }



  /*! Alg. 6.2+6.1 */
  Dart MeshC::CDTInsertSegment(const DartPair& dp,
			       const DartList& trace,
			       triangleSetT& triangles)
  {
    if (!prepareCDT()) return Dart();

    Dart dc;

    std::cout << WHEREAMI << dp;
    std::cout << WHEREAMI << trace;

    std::cout << WHEREAMI
	      << "Segment crosses " << trace.size()
	      << " edges." << std::endl;

    if (trace.size()<=1) {
      std::cout << WHEREAMI
		<< "Too short trace (" << trace.size() << ")."
		<< " Should have been handled by caller."
		<< std::endl;
      return Dart();
    }

    Dart dh;
    Dart d0(dp.first);
    Dart d1(dp.second);
    BoundaryList boundary0;
    BoundaryList boundary1;

    int v0(d0.v());
    int v1(d1.v());
    for (DartList::const_iterator i(trace.begin());
	 i != trace.end(); i++) {
      dh = *i;
      triangles.insert(dh.t());

      if (M_->useX11())
	M_->drawX11triangle(dh.t(),true);

      if (v0 == dh.v()) {
	boundary0.rbegin()->second++;
      } else {
	v0 = dh.v();
	boundary0.push_back(IntPair(v0,1));
      }
      if (v1 == dh.vo()) {
	boundary1.begin()->second++;
      } else {
	v1 = dh.vo();
	boundary1.push_front(IntPair(v1,1));
      }
    }
    dh.alpha2();
    triangles.insert(dh.t());

    if (M_->useX11())
      M_->drawX11triangle(dh.t(),true);

    v0 = d0.v();
    v1 = d1.v();

    std::cout << WHEREAMI << M_->S(v0) << std::endl;
    std::cout << WHEREAMI << M_->S(v1) << std::endl;

    boundary0.push_front(IntPair(v0,-1));
    boundary0.push_back(IntPair(v1,-1));
    boundary1.push_back(IntPair(v0,-1));
    boundary1.push_front(IntPair(v1,-1));

    std::cout << WHEREAMI << "T:  " << triangles << std::endl;
    std::cout << WHEREAMI << "B1: " << boundary1 << std::endl;
    std::cout << WHEREAMI << "B0: " << boundary0 << std::endl;

    //    if (M_->useX11()) {
    //      char msg[] = "Starting segment insertion.";
    //      xtmpl_press_ret(msg);
    //    }

    NOT_IMPLEMENTED;

    BoundaryList::iterator i0, i0_prev, i0_next;
    BoundaryList::reverse_iterator i1, i1_prev, i1_next;

#define	CDTMSG(msg)				\
    std::cout << WHEREAMI << msg << std::endl;				\
    std::cout << WHEREAMI << "B1: " << boundary1 << std::endl;		\
    std::cout << WHEREAMI << "B0: " << boundary0 << std::endl;		\
    std::cout << WHEREAMI << "I1: "					\
	      << *i1_prev << *i1 << *i1_next << std::endl;		\
    std::cout << WHEREAMI << "I0: "					\
	      << *i0_prev << *i0 << *i0_next << std::endl;		\
    std::cout << WHEREAMI << "d0: " << d0 << std::endl;			\
    std::cout << WHEREAMI << "vd0: " << vd0 << std::endl;		\
    std::cout << WHEREAMI << "dh: " << dh << std::endl;

    /* Initialise */
    i0 = boundary0.begin(); i0_prev = i0; i0_next = i0;
    i1 = boundary1.rbegin(); i1_prev = i1; i1_next = i1;
    next(i0_prev,i0,i0_next);
    next(i1_prev,i1,i1_next);

    /* Go to first edge */
    Dart vd0(d0); /* First dart in edge bundle from current vertex. */
    vd0.orbit2();
    while (i1->first != vd0.vo()) next(i1_prev,i1,i1_next);

    dh = vd0;

    while (true) {
      bool swapable = true;
      while (swapable) {
	CDTMSG("");
	/* Find swapable edge */
	dh = vd0;
	for (swapable = dh.isSwapable();
	     (dh.vo() != i0_next->first) && (!swapable);
	     swapable = dh.isSwapable()) {
	  dh.orbit0rev();
	  if (dh.vo() != i0_next->first)
	    while (i1->first != dh.vo()) next(i1_prev,i1,i1_next);
	  CDTMSG("Looking for swapable");
	  std::cout << WHEREAMI << "swapable=" << swapable << std::endl;
	};
	swapable = swapable && (dh.vo() != i0_next->first);
	if (swapable) {
	  /* Swap the edge */
	  bool vd0affected = (dh==vd0); /* vd0 must be updated? */
	  
	  i0->second--;
	  i1->second--;
	  dh = swapEdge(dh);
	  std::cout << WHEREAMI << "dh: " << dh << std::endl;

	  if (dh.vo() != v0) {
	    if (i0_prev->first == dh.vo()) i0_prev->second++;
	    if (i1_prev->first == dh.vo()) i1_prev->second++;
	  }
	  if (dh.v() != v1) {
	    if (i0_next->first == dh.v()) i0_next->second++;
	    if (i1_next->first == dh.v()) i1_next->second++;
	  }

	  if (dh.vo() == v0) {
	    /* Triangle is linked to v0, needs to regenerate d0. */
	    d0 = dh;
	    d0.orbit2();
	    std::cout << WHEREAMI << "d0: " << d0 << std::endl;
	  }

	  CDTMSG("Have swapped, and adjusted counters");

	  if (dh.v() == i0_next->first) {
	    if (dh.vo() == v0) {
	      /* Vertex has been eliminated, and is linked to v0 */
	      i0_next->second--;
	      d0.orbit0();
	      std::cout << WHEREAMI << "d0: " << d0 << std::endl;
	    } else if (dh.vo() == i0_prev->first) {
	      /* Vertex has been eliminated */
	      i0_next->second--;
	      i0_prev->second--;
	    }
	  } else if (dh.vo() == i0_prev->first) {
	    if (dh.v() == v1) {
	      /* Vertex has been eliminated, and is linked to v1 */
	      i1_prev->second--;
	    } else if (dh.v() == i0_next->first) { /* Will not be reached. */
	      /* Vertex has been eliminated */
	      i0_next->second--;
	      i0_prev->second--;
	    }
	  }
	  if ((dh.vo() == i1_prev->first) && (dh.v() == i1_next->first)) {
	    /* Vertex has been eliminated */
	    i1->second = 0;
	    if (i1_next->second >= 0) i1_next->second--;
	    if (i1_prev->second >= 0) i1_prev->second--;
	  }
	  dh.orbit2rev();
	  if (vd0affected)
	    vd0 = dh;
	  swapable = (vd0.vo() != i0_next->first);

	  CDTMSG("Have swapped, and detected parial successes");

	  if (swapable && (i1_next->second >= 0)) {
	    while (i1->first != vd0.vo()) next(i1_prev,i1,i1_next);
	  }
	  if ((i0->second > 0) && (i0_prev->second >= 0)) {
	    prevnext(i0_prev,i0,i0_next);
	  }

	  CDTMSG("Have swapped, and tried to update location");
	  std::cout << WHEREAMI
		    << (swapable ? "(Restart this vertex)" :
			"(Leave vertex)") << std::endl;

	  //	  if (M_->useX11()) {
	  //  char msg[] = "Swapped away an edge.";
	  //  xtmpl_press_ret(msg);
	  // }
      	}
      }

      CDTMSG("Done with vertex, trying to move on.");

      if (d0.vo() == v1) /* Segment has been inserted */
	break; /* This is the only way out of the loop! */

      if (i0->second==0) {
	/* The vertex was eliminated, start from beginning. */
	i0 = boundary0.begin(); i0_prev = i0; i0_next = i0;
	i1 = boundary1.rbegin(); i1_prev = i1; i1_next = i1;
	next(i0_prev,i0,i0_next);
	next(i1_prev,i1,i1_next);
	
	/* Go to first edge */
	vd0 = d0; /* First dart in edge bundle from current vertex. */
	vd0.orbit2();
	while (i1->first != vd0.vo()) next(i1_prev,i1,i1_next);

	CDTMSG("Vertex eliminated, start from beginning:");
      } else { /* The vertex was not eliminated, go to next. */
	next(i0_prev,i0,i0_next);
	dh.orbit2(); /* First dart in edge bundle from current vertex. */
	vd0 = dh;
	while (i1->first != vd0.vo()) next(i1_prev,i1,i1_next);
	CDTMSG("Vertex not eliminated, go to next:");
      }

      //      if (M_->useX11()) {
      //	char msg[] = "Tried to eliminate vertex.";
      //	xtmpl_press_ret(msg);
      // }
      
    }

    std::cout << "Segment inserted:" << std::endl;
    std::cout << WHEREAMI << "B1: " << boundary1 << std::endl;
    std::cout << WHEREAMI << "B0: " << boundary0 << std::endl;
    std::cout << WHEREAMI << "d0: " << d0 << std::endl;

    dc = d0;

    std::cout << WHEREAMI
	      << "Segment dart " << dc << std::endl;

    return dc;
  }



  Dart MeshC::CDTInsertSegment(const int v0, const int v1,
			       triangleSetT& triangles)
  {
    if (!prepareCDT()) return Dart();
    std::cout << WHEREAMI << "Inserting segment ("
	      << v0 << "," << v1 << ")" << std::endl;
    if (v0 == v1) return Dart();

    DartList trace;
    Dart dh(M_->locateVertex(Dart(),v0));
    if (dh.isnull()) {
      std::cout << WHEREAMI << "Originating vertex not found "
		<< v0 << std::endl;
      return Dart();
    }
    DartPair dhp(M_->tracePath(dh,M_->S(v1),v1,&trace));
    if (dhp.second.isnull()) {
      std::cout << WHEREAMI << "Endpoint vertex not found ("
		<< v0 << "," << v1 << ") "
		<< M_->S(v0) << ", " << M_->S(v0) << std::endl;
      return Dart();
    }

    Dart dh0(dhp.first);
    Dart dh1(dhp.second);

    /* Already an edge? */
    if (dh0.t() == dh1.t()) {
      std::cout << WHEREAMI << "Segment already an edge. Darts: "
		<< dhp;
      dh0.alpha0();
      if (v1 == dh0.v()) {
	dh0.alpha0();
	return dh0;
      } else {
	dh1.orbit1();
	return dh1;
      }
    }

    /* Only one crossing edge, swap directly. */
    if (trace.size()==1) {
      Dart ds = swapEdge(*trace.begin());
      if (ds.v() == v1) ds.orbit1();
      if (ds.v() != v0) ds = Dart();

      std::cout << WHEREAMI
		<< "Segment dart " << ds << std::endl;

      return ds;
    }

    return CDTInsertSegment(dhp,trace,triangles);
  }








  Dart MeshC::CDTSegment(const bool boundary, const int v0, const int v1)
  {
    if (!prepareCDT()) return Dart();

    triangleSetT triangles;
    Dart ds(CDTInsertSegment(v0,v1,triangles));
    if (ds.isnull()) {
      std::cout << WHEREAMI << (boundary ? "Boundary" : "Interior")
		<< " segment not inserted ("
		<< v0 << "," << v1 << ")" << std::endl;
      return ds;
    }
    if (boundary)
      boundary_.insert(ds);
    else
      interior_.insert(ds);
    LOP(triangles);
    std::cout << WHEREAMI << (boundary ? "Boundary" : "Interior")
	      << " segment inserted "
	      << ds << std::endl;
    return ds;
  }


  bool MeshC::buildCDT()
  {
    if (!prepareCDT()) return false;

    constrListT::iterator ci_next;
    for (constrListT::iterator ci = constr_boundary_.begin();
	 ci != constr_boundary_.end(); ) {
      if (!CDTSegment(true,ci->first,ci->second).isnull()) {
	ci_next = ci;
	ci_next++;
	ci = constr_boundary_.erase(ci);
	ci = ci_next;
      } else {
	ci++;
      }
    }
    for (constrListT::iterator ci = constr_interior_.begin();
	 ci != constr_interior_.end(); ) {
      if (!CDTSegment(false,ci->first,ci->second).isnull()) {
	ci_next = ci;
	ci_next++;
	ci = constr_interior_.erase(ci);
	ci = ci_next;
      } else {
	ci++;
      }
    }

    std::cout << WHEREAMI << "Boundary segments after CDT:" << std::endl << boundary_;
    std::cout << WHEREAMI << "Interior segments after CDT:" << std::endl << interior_;

    M_->redrawX11("CDT finished");

    return (constr_boundary_.empty() && constr_interior_.empty());
  };









  bool MeshC::buildRCDTlookahead(MCQsegm* segm, const Point& c)
  {
    std::cout << WHEREAMI << "Checking for potentially encroached segments at ("
	      << c[0] << ',' << c[1] << ',' << c[2] << ")" << std::endl;
    for (MCQ::const_iterator ci = segm->begin();
	 ci != segm->end(); ci++) {
      Dart dhc(ci->first);
      double encr = M_->edgeEncroached(dhc,c);
      if (encr>0.0) {
	std::cout << WHEREAMI << "Potentially encroached segment: "
		  << dhc << " "
		  << encr << std::endl;
	bisectEdgeDelaunay(dhc);
	// if (!bisectEdgeDelaunay(dhc).isnull())
	//   xtmpl_press_ret("Potentialy encroached segment split.");
	// else
	//   xtmpl_press_ret("Failed to split potentialy encroached segment.");
	return false;
      }
    }
    return true;
  }



  bool MeshC::buildRCDT()
  {
    if (state_<State_RCDT)
      return false; /* ERROR: RCDT not initialised. */

    std::cout << WHEREAMI << "Encroached boundary segments before RCDT:" << std::endl
	      << boundary_;
    std::cout << WHEREAMI << "Encroached interior segments before RCDT:" << std::endl
	      << interior_;
    std::cout << WHEREAMI << "Skinny triangles before RCDT:" << std::endl << skinny_;
    std::cout << WHEREAMI << "Big triangles before RCDT:" << std::endl << big_;

    Dart dh;

    int loop = 0;
    while (!(boundary_.emptyQ() && interior_.emptyQ() &&
	     skinny_.emptyQ() && big_.emptyQ())) {
      /* Temporary failsafe exit: */
      /*
      loop++;
      if (loop>50000) return false;
      */

      std::cout << WHEREAMI << "RCDT(" << loop << "): (Bo,In,Sk,Bi) = ("
		<< boundary_.countQ() << ","
		<< interior_.countQ() << ","
		<< skinny_.countQ() << ","
		<< big_.countQ() << ")" << std::endl;

      dh = boundary_.quality();
      if (!dh.isnull()) {
	std::cout << WHEREAMI << "Encroached boundary segment: "
		  << dh << " "
		  << big_.quality(dh) << std::endl;
	bisectEdgeDelaunay(dh);
	//	if (!bisectEdgeDelaunay(dh).isnull())
	//	  xtmpl_press_ret("Boundary segment has been split");
	//	else
	//	  xtmpl_press_ret("Boundary segment split failed");
	continue;
      }
      
      dh = interior_.quality();
      if (!dh.isnull()) {
	std::cout << WHEREAMI << "Encroached interior segment: "
		  << dh << " "
		  << big_.quality(dh) << std::endl;
	bisectEdgeDelaunay(dh);
	// if (!bisectEdgeDelaunay(dh).isnull())
	//   xtmpl_press_ret("Interior segment has been split");
	// else
	//   xtmpl_press_ret("Interior segment split failed");
	continue;
      }

      //      xtmpl_press_ret("No segments need splitting.");

      dh = skinny_.quality();
      if (!dh.isnull()) {
	std::cout << WHEREAMI << "Skinny triangle: "
		  << dh << " "
		  << skinny_.quality(dh) << std::endl;
	Point c;
	calcSteinerPoint(dh,c);
	if ((!buildRCDTlookahead(&boundary_,c)) ||
	    (!buildRCDTlookahead(&interior_,c)))
	  continue;
	if (insertNode(addVertices(&c,1),dh).isnull()) {
	// if (killTriangle(dh))
	//   xtmpl_press_ret("Skinny triangle has been eliminated");
	// else
	  char msg[] = "Skinny triangle elimination failed";
	  xtmpl_press_ret(msg);
	} else {
	  //	  if (M_->nV()>811)
	  //	    xtmpl_press_ret("Skinny triangle elimination succeeded");
	}
	continue;
      }
      
      dh = big_.quality();
      if (!dh.isnull()) {
	std::cout << WHEREAMI << "Big triangle: "
		  << dh << " "
		  << big_.quality(dh) << std::endl;
	Point c;
	calcSteinerPoint(dh,c);
	if ((!buildRCDTlookahead(&boundary_,c)) ||
	    (!buildRCDTlookahead(&interior_,c)))
	  continue;
	insertNode(addVertices(&c,1),dh);
	// if (!insertNode(addVertices(&c,1),dh).isnull())
	//   xtmpl_press_ret("Big triangle has been eliminated");
	// else
	//   xtmpl_press_ret("Big triangle elimination failed");
	continue;
      }
      
    }

    M_->redrawX11("RCDT finished");

    return true;
  };

  bool MeshC::RCDT(double skinny_limit, double big_limit)
  {
    if (!prepareRCDT(skinny_limit,big_limit)) return false;
    return buildRCDT();
  };


  /*!
    Flood-fill algorithm for exterior triangle removal:
    \verbatim
    1. For each boundary segment
    2.   Add a possible exterior triangle to the "exterior set" Ext.
    3.   Unlink the edge.
    4. While Ext is not empty, let t in Ext
    5.   Add any triangle linked to t to Ext.
    6.   Remove t.
    \endverbatim

    Step 1. is delicate, since boundary segments will be added and
    removed in step 3.  Solved by finding the previous segment again,
    and then moving to the next.

    TODO: Remove exterior points at the end of the vertex list.\n

    TODO: Allow optional vertex reordering.
   */
  bool MeshC::PruneExterior()
  {
    if (state_ < State_CDT) {
      /* Since there are no constraints at this state, no exterior
	 needs to be pruned, but go to state State_CDT anyway. */
      prepareCDT();
      is_pruned_ = true;
      return true;
    }
    is_pruned_ = true;
    
    Dart d0, dh;
    triangleSetT ext;

    /* Unlink the exterior. */
    for (MCQsegm::const_iterator boundary_segm_i = boundary_.begin();
	 boundary_segm_i != boundary_.end();
	 boundary_segm_i = ++boundary_.find(d0)) {
      d0 = boundary_segm_i->first;
      if (!d0.onBoundary()) {
	dh = d0;
	dh.orbit1();
	ext.insert(dh.t());
	unlinkEdge(d0);
      }
    }

    /* Make sure useVT is off: */
    bool M_useVT = M_->useVT();
    M_->useVT(false);

    /* Remove exterior triangles. */
    int t_relocated;
    triangleSetT::iterator ext_j;
    for (triangleSetT::iterator ext_i = ext.begin();
	 ext_i != ext.end();
	 ext_i = ext.begin()) {
      dh = Dart(*M_,*ext_i);
      if (!dh.onBoundary()) ext.insert(dh.tadj());
      dh.orbit2();
      if (!dh.onBoundary()) ext.insert(dh.tadj());
      dh.orbit2();
      if (!dh.onBoundary()) ext.insert(dh.tadj());
      t_relocated = removeTriangle(dh);
      if ((ext_j = ext.find(t_relocated)) != ext.end())
	ext.erase(ext_j);
      else
	ext.erase(ext_i);

      std::cout << WHEREAMI << ext << std::endl;
    }

    /* Restore useVT-state: */
    M_->useVT(M_useVT);

    std::cout << WHEREAMI << "Boundary segments after pruning: "
	      << boundary_;

    M_->redrawX11("PruneExterior finished");

    return true;
  };








  Dart MeshC::swapEdge(const Dart& d, MCQswapable& swapable)
  {
    if (!swapable.swapable(d)) {
      /* Not allowed to swap. */
      std::cout << WHEREAMI << "Not allowed to swap dart " << std::endl
		<< d << std::endl;
      return d;
    }
    
    /* Collect swapable data */
    bool edge_list[4];
    Dart dh(d);
    swapable.erase(dh);
    dh.orbit2rev();
    if ((edge_list[1] = swapable.found(dh))) swapable.erase(dh);
    dh.orbit2rev();
    if ((edge_list[2] = swapable.found(dh))) swapable.erase(dh);
    dh.orbit0().orbit2rev();
    if ((edge_list[3] = swapable.found(dh))) swapable.erase(dh);
    dh.orbit2rev();
    if ((edge_list[0] = swapable.found(dh))) swapable.erase(dh);

    // std::cout << WHEREAMI << "LOP edge list: ("
    // 	      << edge_list[0] << ","
    // 	      << edge_list[1] << ","
    // 	      << edge_list[2] << ","
    // 	      << edge_list[3] << ")" << std::endl;
    
    Dart dnew(swapEdge(d));
    if (dh == dnew) {
      /* ERROR: this should not happen. */
      std::cout << WHEREAMI << "Edge swap appears to have failed!" << std::endl;
      return dnew;
    }

    /* Reassemble swapable data */
    dh = dnew;
    swapable.insert(dh);
    dh.orbit2();
    if (edge_list[1]) swapable.insert(dh);
    dh.orbit2();
    if (edge_list[0]) swapable.insert(dh);
    dh.orbit2().orbit0rev();
    if (edge_list[3]) swapable.insert(dh);
    dh.orbit2();
    if (edge_list[2]) swapable.insert(dh);

    //    std::cout << WHEREAMI << "Edge swapped:" << std::endl;

    return dnew;
  }

  Dart MeshC::swapEdge(const Dart& d)
  {
    if (state_ < State_CDT) {
      return M_->swapEdge(d);
    }

    if (boundary_.segm(d) || interior_.segm(d)) {
      /* ERROR: Not allowed to swap. */
      std::cout << WHEREAMI << "ERROR: Not allowed to swap dart " << std::endl
		<< d << std::endl;
      return d;
    }

    /* Collect CDT data */
    bool segm_b[4];
    bool segm_i[4];
    Dart dh(d);
    if (state_>=State_CDT) {
    dh.orbit2rev();
    if ((segm_b[1] = boundary_.found(dh))) boundary_.erase(dh);
    if ((segm_i[1] = interior_.found(dh))) interior_.erase(dh);
    dh.orbit2rev();
    if ((segm_b[2] = boundary_.found(dh))) boundary_.erase(dh);
    if ((segm_i[2] = interior_.found(dh))) interior_.erase(dh);
    dh.orbit0().orbit2rev();
    if ((segm_b[3] = boundary_.found(dh))) boundary_.erase(dh);
    if ((segm_i[3] = interior_.found(dh))) interior_.erase(dh);
    dh.orbit2rev();
    if ((segm_b[0] = boundary_.found(dh))) boundary_.erase(dh);
    if ((segm_i[0] = interior_.found(dh))) interior_.erase(dh);
    }

    if (state_>=State_RCDT) {
      /* Collect RCDT data */
      dh = d;
      skinny_.erase(dh);
      big_.erase(dh);
      dh.orbit1();
      skinny_.erase(dh);
      big_.erase(dh);
    }

    Dart dnew(M_->swapEdge(d));

    if (state_>=State_CDT) {
    /* Reassemble CDT data */
    dh = dnew;
    dh.orbit2();
    boundary_.update(dh); if (segm_b[1]) boundary_.insert(dh);
    interior_.update(dh); if (segm_i[1]) interior_.insert(dh);
    dh.orbit2();
    boundary_.update(dh); if (segm_b[0]) boundary_.insert(dh);
    interior_.update(dh); if (segm_i[0]) interior_.insert(dh);
    dh.orbit2().orbit0rev();
    boundary_.update(dh); if (segm_b[3]) boundary_.insert(dh);
    interior_.update(dh); if (segm_i[3]) interior_.insert(dh);
    dh.orbit2();
    boundary_.update(dh); if (segm_b[2]) boundary_.insert(dh);
    interior_.update(dh); if (segm_i[2]) interior_.insert(dh);
    }

    if (state_>=State_RCDT) {
      /* Reassemble RCDT data */
      dh = dnew;
      skinny_.insert(dh);
      big_.insert(dh);
      dh.orbit1();
      skinny_.insert(dh);
      big_.insert(dh);
    }

    //    std::cout << WHEREAMI << "Edge swapped, boundary segments:" << std::endl
    //	      << boundary_;

    return dnew;
  }

  Dart MeshC::splitEdge(const Dart& d, int v)
  {
    if (state_ < State_CDT) {
      return M_->splitEdge(d,v);
    }

    /* Collect CDT data */
    Dart dh(d);
    bool segm_b[6];
    bool segm_i[6];
    if (state_>=State_CDT) {
    for (int i=0;i<3;i++) {
      if ((segm_b[i] = boundary_.found(dh))) boundary_.erase(dh);
      if ((segm_i[i] = interior_.found(dh))) interior_.erase(dh);
      dh.orbit2();
    }
    if (!dh.onBoundary()) {
      dh.orbit1();
      for (int i=3;i<6;i++) {
	if ((segm_b[i] = boundary_.found(dh))) boundary_.erase(dh);
	if ((segm_i[i] = interior_.found(dh))) interior_.erase(dh);
	dh.orbit2();
      }
    }
    }

    if (state_>=State_RCDT) {
      /* Collect RCDT data */
      dh = d;
      skinny_.erase(dh);
      big_.erase(dh);
      if (!dh.onBoundary()) {
	dh.orbit1();
	skinny_.erase(dh);
	big_.erase(dh);
      }
    }

    Dart dnew(M_->splitEdge(d,v));

    if (state_>=State_CDT) {
    /* Reassemble CDT data */
    dh = dnew;
    boundary_.update(dh); if (segm_b[0]) boundary_.insert(dh);
    interior_.update(dh); if (segm_i[0]) interior_.insert(dh);
    dh.orbit2();
    boundary_.update(dh); if (segm_b[1]) boundary_.insert(dh);
    interior_.update(dh); if (segm_i[1]) interior_.insert(dh);
    dh.orbit2().orbit0rev();
    boundary_.update(dh); if (segm_b[2]) boundary_.insert(dh);
    interior_.update(dh); if (segm_i[2]) interior_.insert(dh);
    dh.orbit2();
    boundary_.update(dh); if (segm_b[0]) boundary_.insert(dh);
    interior_.update(dh); if (segm_i[0]) interior_.insert(dh);
    if (!dh.onBoundary()) {
      dh.orbit1();
      boundary_.update(dh); if (segm_b[3]) boundary_.insert(dh);
      interior_.update(dh); if (segm_i[3]) interior_.insert(dh);
      dh.orbit2();
      boundary_.update(dh); if (segm_b[4]) boundary_.insert(dh);
      interior_.update(dh); if (segm_i[4]) interior_.insert(dh);
      dh.orbit2().orbit0rev();
      boundary_.update(dh); if (segm_b[5]) boundary_.insert(dh);
      interior_.update(dh); if (segm_i[5]) interior_.insert(dh);
      dh.orbit2();
      boundary_.update(dh); if (segm_b[3]) boundary_.insert(dh);
      interior_.update(dh); if (segm_i[3]) interior_.insert(dh);
    }
    }

    if (state_>=State_RCDT) {
      /* Reassemble RCDT data */
      dh = dnew;
      skinny_.insert(dh);
      big_.insert(dh);
      dh.orbit0();
      skinny_.insert(dh);
      big_.insert(dh);
      if (!dnew.onBoundary()) {
	dh.orbit0();
	skinny_.insert(dh);
	big_.insert(dh);
	dh.orbit0();
	skinny_.insert(dh);
	big_.insert(dh);
      }
    }

    //    std::cout << WHEREAMI << "Edge split, boundary segments:" << std::endl
    //	      << boundary_;

    //    xtmpl_press_ret("Edge has been split");

    return dnew;
  }

  Dart MeshC::splitTriangle(const Dart& d, int v)
  {
    if (state_ < State_CDT) {
      return M_->splitTriangle(d,v);
    }

    /* Collect CDT data */
    Dart dh(d);
    bool segm_b[3];
    bool segm_i[3];
    if (state_>=State_CDT) {
    for (int i=0;i<3;i++) {
      if ((segm_b[i] = boundary_.found(dh))) boundary_.erase(dh);
      if ((segm_i[i] = interior_.found(dh))) interior_.erase(dh);
      dh.orbit2();
    }
    }

    if (state_>=State_RCDT) {
      /* Collect RCDT data */
      skinny_.erase(d);
      big_.erase(d);
    }

    Dart dnew(M_->splitTriangle(d,v));

    /* Reassebmle CDT data */
    if (state_>=State_CDT) {
    dh = dnew;
    for (int i=0;i<3;i++) {
      dh.orbit2();
      boundary_.update(dh); if (segm_b[i]) boundary_.insert(dh);
      interior_.update(dh); if (segm_i[i]) interior_.insert(dh);
      dh.orbit2rev().orbit0();
    }
    }

    if (state_>=State_RCDT) {
      /* Reassemble RCDT data */
      dh = dnew;
      skinny_.insert(dh);
      big_.insert(dh);
      dh.orbit0();
      skinny_.insert(dh);
      big_.insert(dh);
      dh.orbit0();
      skinny_.insert(dh);
      big_.insert(dh);
    }

    //    std::cout << WHEREAMI << "Triangle split, boundary segments:" << std::endl
    //	      << boundary_;

    return dnew;
  }




  /*!  */
  void MeshC::unlinkEdge(Dart& d)
  {
    if (state_<State_CDT) {
      d.unlinkEdge();
      return;
    }

    Dart dh(d);
    bool onboundary = d.onBoundary();
    if (!onboundary) {
      dh.orbit1();
      if (interior_.found(dh)) interior_.erase(dh);
    }
    if (interior_.found(d)) interior_.erase(d);
    
    d.unlinkEdge();
        
    if (!onboundary) {
      boundary_.erase(dh);
      boundary_.insert(dh);
    }
    boundary_.erase(d);
    boundary_.insert(d);

    return;
  }

  /*!  */
  int MeshC::removeTriangle(Dart& d)
  {
    if (state_ < State_CDT) {
      return M_->removeTriangle(d.t());
    }

    Dart dh(d);
    interior_.erase(dh);
    boundary_.erase(dh);
    if (!dh.onBoundary()) {
      dh.orbit1();
      interior_.erase(dh);
      boundary_.insert(dh);
      dh.orbit1();
    }
    dh.orbit2();
    interior_.erase(dh);
    boundary_.erase(dh);
    if (!dh.onBoundary()) {
      dh.orbit1();
      interior_.erase(dh);
      boundary_.insert(dh);
      dh.orbit1();
    }
    dh.orbit2();
    interior_.erase(dh);
    boundary_.erase(dh);
    if (!dh.onBoundary()) {
      dh.orbit1();
      interior_.erase(dh);
      boundary_.insert(dh);
      dh.orbit1();
    }

    int t_removed = d.t();
    int t_relocated = M_->removeTriangle(t_removed);
    
    dh = Dart(*M_,t_removed,1,0);
    Dart dh_old = Dart(*M_,t_relocated,1,0);
    if (boundary_.found(dh_old)) {
      boundary_.erase(dh_old); boundary_.insert(dh);
    }
    if (interior_.found(dh_old)) {
      interior_.erase(dh_old); interior_.insert(dh);
    }
    dh.orbit2(); dh_old.orbit2();
    if (boundary_.found(dh_old)) {
      boundary_.erase(dh_old); boundary_.insert(dh);
    }
    if (interior_.found(dh_old)) {
      interior_.erase(dh_old); interior_.insert(dh);
    }
    dh.orbit2(); dh_old.orbit2();
    if (boundary_.found(dh_old)) {
      boundary_.erase(dh_old); boundary_.insert(dh);
    }
    if (interior_.found(dh_old)) {
      interior_.erase(dh_old); interior_.insert(dh);
    }
    
    return t_relocated;
  }







  bool MeshC::isSegment(const Dart& d) const
  {
    if (state_<State_CDT) /* No segments */
      return false;

    return (boundary_.segm(d) || interior_.segm(d));
  }







  std::ostream& operator<<(std::ostream& output, const MCQ& Q)
  {
    if (Q.empty()) return output;
    output << "N,n = " << Q.count() << "," << Q.countQ() << std::endl;
    for (MCQ::map_type::const_iterator qi = Q.darts_.begin();
	 qi != Q.darts_.end(); qi++) {
      output << ' ' << qi->first
	     << ' ' << std::scientific << qi->second
	     << ' ' << Q.foundQ(qi->first)
	     << std::endl;
    }
    return output;
  }

  std::ostream& operator<<(std::ostream& output, const DartPair& dp)
  {
    output << "d0=(" << dp.first << ") d1=("
	   << dp.second << ")" << std::endl;
    return output;
  }

  std::ostream& operator<<(std::ostream& output, const DartList& ds)
  {
    output << "n = " << ds.size() << std::endl;
    if (ds.empty()) return output;
    for (DartList::const_iterator di = ds.begin();
	 di != ds.end(); di++) {
      output << ' ' << *di << std::endl;
    }
    return output;
  }

  std::ostream& operator<<(std::ostream& output, const std::set<int>& il)
  {
    output << "(n = " << il.size() << ")";
    if (il.empty()) return output;
    for (std::set<int>::const_iterator i = il.begin();
	 i != il.end(); i++) {
      output << ' ' << *i;
    }
    return output;
  }

  std::ostream& operator<<(std::ostream& output, const std::list<int>& il)
  {
    output << "(n = " << il.size() << ")";
    if (il.empty()) return output;
    for (std::list<int>::const_iterator i = il.begin();
	 i != il.end(); i++) {
      output << ' ' << *i;
    }
    return output;
  }

  std::ostream& operator<<(std::ostream& output,
			   const IntPair& p)
  {
    output << "(" << p.first << "," << p.second << ")";
    return output;
  }

  std::ostream& operator<<(std::ostream& output,
			   const std::list<IntPair>& il)
  {
    output << "(n = " << il.size() << ")";
    if (il.empty()) return output;
    for (std::list<IntPair>::const_iterator i = il.begin();
	 i != il.end(); i++) {
      output << " " << *i;
    }
    return output;
  }


} /* namespace fmesh */
