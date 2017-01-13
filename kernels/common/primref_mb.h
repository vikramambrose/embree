// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "default.h"
#include "scene.h"

#define MBLUR_BIN_LBBOX 0

namespace embree
{
  struct UserPrimRefData
  {
    __forceinline UserPrimRefData(Scene* scene, BBox1f time_range)
      : scene(scene), time_range(time_range) {}

    Scene* scene;
    BBox1f time_range;
  };

#if MBLUR_BIN_LBBOX

  /*! A primitive reference stores the bounds of the primitive and its ID. */
  struct __aligned(32) PrimRefMB
  {
    typedef LBBox3fa BBox;

    __forceinline PrimRefMB () {}

    __forceinline PrimRefMB (const LBBox3fa& lbounds_i, unsigned int activeTimeSegments, unsigned int totalTimeSegments, unsigned int geomID, unsigned int primID)
      : lbounds(lbounds_i)
    {
      assert(activeTimeSegments > 0);
      lbounds.bounds0.lower.a = geomID;
      lbounds.bounds0.upper.a = primID;
      lbounds.bounds1.lower.a = activeTimeSegments;
      lbounds.bounds1.upper.a = totalTimeSegments;
    }

    __forceinline PrimRefMB (const LBBox3fa& lbounds_i, unsigned int activeTimeSegments, unsigned int totalTimeSegments, size_t id)
      : lbounds(lbounds_i)
    {
      assert(activeTimeSegments > 0);
#if defined(__X86_64__)
      lbounds.bounds0.lower.u = id & 0xFFFFFFFF;
      lbounds.bounds0.upper.u = (id >> 32) & 0xFFFFFFFF;
#else
      lbounds.bounds0.lower.u = id;
      lbounds.bounds0.upper.u = 0;
#endif
      lbounds.bounds1.lower.a = activeTimeSegments;
      lbounds.bounds1.upper.a = totalTimeSegments;
    }

    /*! returns bounds for binning */
    __forceinline LBBox3fa bounds() const {
      return lbounds;
    }

    /*! returns the number of time segments of this primref */
    __forceinline unsigned size() const {
      return lbounds.bounds1.lower.a;
    }

    __forceinline unsigned totalTimeSegments() const {
      return lbounds.bounds1.upper.a;
    }

    /*! returns center for binning */
    __forceinline Vec3fa binCenter() const {
      //return center2(lbounds.interpolate(0.0f));
      return center2(lbounds.interpolate(0.5f));
      //return center2(lbounds.interpolate(1.0f));
    }

    /*! returns bounds and centroid used for binning */
    __forceinline void binBoundsAndCenter(LBBox3fa& bounds_o, Vec3fa& center_o) const
    {
      bounds_o = bounds();
      center_o = binCenter();
    }

    __forceinline void binBoundsAndCenter(BBox3fa& bounds_o, Vec3fa& center_o) const
    {
      bounds_o = lbounds.interpolate(0.5f);
      center_o = center2(bounds_o);
    }

    /*! returns center for binning */
    __forceinline Vec3fa binCenter(const AffineSpace3fa& space, void* user) const // only called by bezier msmblur builder
    {
      Scene* scene = ((UserPrimRefData*) user)->scene;
      BBox1f time_range = ((UserPrimRefData*) user)->time_range;
      BezierCurves* mesh = (BezierCurves*) scene->get(geomID());
      LBBox3fa lbounds = mesh->linearBounds(space,primID(),time_range);
      return center2(lbounds.interpolate(0.5f));
    }

    /*! returns bounds and centroid used for binning */
    __forceinline void binBoundsAndCenter(LBBox3fa& bounds_o, Vec3fa& center_o, const AffineSpace3fa& space, void* user) const // only called by bezier msmblur builder
    {
      Scene* scene = ((UserPrimRefData*) user)->scene;
      BBox1f time_range = ((UserPrimRefData*) user)->time_range;
      BezierCurves* mesh = (BezierCurves*) scene->get(geomID());
      LBBox3fa lbounds = mesh->linearBounds(space,primID(),time_range);
      bounds_o = lbounds;
      center_o = center2(lbounds.interpolate(0.5f));
    }

    /*! returns the geometry ID */
    __forceinline unsigned geomID() const {
      return lbounds.bounds0.lower.a;
    }

    /*! returns the primitive ID */
    __forceinline unsigned primID() const {
      return lbounds.bounds0.upper.a;
    }

    /*! returns an size_t sized ID */
    __forceinline size_t ID() const {
#if defined(__X86_64__)
      return size_t(lbounds.bounds0.lower.u) + (size_t(lbounds.bounds0.upper.u) << 32);
#else
      return size_t(lbounds.bounds0.lower.u);
#endif
    }

    /*! special function for operator< */
    __forceinline uint64_t ID64() const {
      return (((uint64_t)primID()) << 32) + (uint64_t)geomID();
    }

    /*! allows sorting the primrefs by ID */
    friend __forceinline bool operator<(const PrimRefMB& p0, const PrimRefMB& p1) {
      return p0.ID64() < p1.ID64();
    }

    /*! Outputs primitive reference to a stream. */
    friend __forceinline std::ostream& operator<<(std::ostream& cout, const PrimRefMB& ref) {
      return cout << "{ lbounds = " << ref.lbounds << ", geomID = " << ref.geomID() << ", primID = " << ref.primID() << " }";
    }

  public:
    LBBox3fa lbounds;
  };

#else

  /*! A primitive reference stores the bounds of the primitive and its ID. */
  struct __aligned(16) PrimRefMB
  {
    typedef BBox3fa BBox;

    __forceinline PrimRefMB () {}

    __forceinline PrimRefMB (const LBBox3fa& bounds, unsigned int activeTimeSegments, unsigned int totalTimeSegments, unsigned int geomID, unsigned int primID)
      : bbox(bounds.interpolate(0.5f))
    {
      assert(activeTimeSegments > 0);
      bbox.lower.a = geomID;
      bbox.upper.a = primID;
      num.x = activeTimeSegments;
      num.y = totalTimeSegments;
    }

    __forceinline PrimRefMB (const LBBox3fa& bounds, unsigned int activeTimeSegments, unsigned int totalTimeSegments, size_t id)
      : bbox(bounds.interpolate(0.5f))
    {
      assert(activeTimeSegments > 0);
#if defined(__X86_64__)
      bbox.lower.u = id & 0xFFFFFFFF;
      bbox.upper.u = (id >> 32) & 0xFFFFFFFF;
#else
      bbox.lower.u = id;
      bbox.upper.u = 0;
#endif
      num.x = activeTimeSegments;
      num.y = totalTimeSegments;
    }

    /*! returns bounds for binning */
    __forceinline BBox3fa bounds() const {
      return bbox;
    }

    /*! returns the number of time segments of this primref */
    __forceinline unsigned size() const { 
      return num.x;
    }

    __forceinline unsigned totalTimeSegments() const { 
      return num.y;
    }

    /*! returns center for binning */
    __forceinline Vec3fa binCenter() const {
      return center2(bounds());
    }

    /*! returns bounds and centroid used for binning */
    __forceinline void binBoundsAndCenter(BBox3fa& bounds_o, Vec3fa& center_o) const
    {
      bounds_o = bounds();
      center_o = center2(bounds());
    }

    /*! returns center for binning */
    __forceinline Vec3fa binCenter(const AffineSpace3fa& space, void* user) const // only called by bezier msmblur builder
    {
      Scene* scene = ((UserPrimRefData*) user)->scene;
      BBox1f time_range = ((UserPrimRefData*) user)->time_range;
      BezierCurves* mesh = (BezierCurves*) scene->get(geomID());
      LBBox3fa lbounds = mesh->linearBounds(space,primID(),time_range);
      return center2(lbounds.interpolate(0.5f));
    }

    /*! returns bounds and centroid used for binning */
    __forceinline void binBoundsAndCenter(BBox3fa& bounds_o, Vec3fa& center_o, const AffineSpace3fa& space, void* user) const // only called by bezier msmblur builder
    {
      Scene* scene = ((UserPrimRefData*) user)->scene;
      BBox1f time_range = ((UserPrimRefData*) user)->time_range;
      BezierCurves* mesh = (BezierCurves*) scene->get(geomID());
      LBBox3fa lbounds = mesh->linearBounds(space,primID(),time_range);
      bounds_o = lbounds.interpolate(0.5f);
      center_o = center2(bounds_o);
    }

    /*! returns the geometry ID */
    __forceinline unsigned geomID() const { 
      return bbox.lower.a;
    }

    /*! returns the primitive ID */
    __forceinline unsigned primID() const { 
      return bbox.upper.a;
    }

    /*! returns an size_t sized ID */
    __forceinline size_t ID() const { 
#if defined(__X86_64__)
      return size_t(bbox.lower.u) + (size_t(bbox.upper.u) << 32);
#else
      return size_t(bbox.lower.u);
#endif
    }

    /*! special function for operator< */
    __forceinline uint64_t ID64() const {
      return (((uint64_t)primID()) << 32) + (uint64_t)geomID();
    }
    
    /*! allows sorting the primrefs by ID */
    friend __forceinline bool operator<(const PrimRefMB& p0, const PrimRefMB& p1) {
      return p0.ID64() < p1.ID64();
    }

    /*! Outputs primitive reference to a stream. */
    friend __forceinline std::ostream& operator<<(std::ostream& cout, const PrimRefMB& ref) {
      return cout << "{ bounds = " << ref.bounds() << ", geomID = " << ref.geomID() << ", primID = " << ref.primID() << " }";
    }

  public:
    BBox3fa bbox; // bounds, geomID, primID
    Vec3ia num;   // activeTimeSegments, totalTimeSegments
  };

#endif
}
