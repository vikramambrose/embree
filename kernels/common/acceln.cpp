// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "acceln.h"
#include "ray.h"
#include "../../include/embree3/rtcore_ray.h"
#include "../../common/algorithms/parallel_for.h"

namespace embree
{
  AccelN::AccelN () 
    : Accel(AccelData::TY_ACCELN), accels(nullptr), validAccels(nullptr) {}

  AccelN::~AccelN() 
  {
    for (size_t i=0; i<accels.size(); i++)
      delete accels[i];
  }

  void AccelN::add(Accel* accel) 
  {
    assert(accel);
    if (accels.size() == accels.max_size())
      throw_RTCError(RTC_ERROR_UNKNOWN,"internal error: AccelN too small");
    
    accels.push_back(accel);
  }

  void AccelN::init() 
  {
    for (size_t i=0; i<accels.size(); i++)
      delete accels[i];
    
    accels.clear();
    validAccels.clear();
  }
  
  void AccelN::intersect (Accel::Intersectors* This_in, RTCRayHit& ray, IntersectContext* context) 
  {
    AccelN* This = (AccelN*)This_in->ptr;
    for (size_t i=0; i<This->validAccels.size(); i++)
      This->validAccels[i]->intersectors.intersect(ray,context);
  }

  void AccelN::intersect4 (const void* valid, Accel::Intersectors* This_in, RTCRayHit4& ray, IntersectContext* context) 
  {
    AccelN* This = (AccelN*)This_in->ptr;
    for (size_t i=0; i<This->validAccels.size(); i++)
      This->validAccels[i]->intersectors.intersect4(valid,ray,context);
  }

  void AccelN::intersect8 (const void* valid, Accel::Intersectors* This_in, RTCRayHit8& ray, IntersectContext* context) 
  {
    AccelN* This = (AccelN*)This_in->ptr;
    for (size_t i=0; i<This->validAccels.size(); i++)
      This->validAccels[i]->intersectors.intersect8(valid,ray,context);
  }

  void AccelN::intersect16 (const void* valid, Accel::Intersectors* This_in, RTCRayHit16& ray, IntersectContext* context) 
  {
    AccelN* This = (AccelN*)This_in->ptr;
    for (size_t i=0; i<This->validAccels.size(); i++)
      This->validAccels[i]->intersectors.intersect16(valid,ray,context);
  }

  void AccelN::intersectN (Accel::Intersectors* This_in, RayHitK<VSIZEX>** ray, const size_t N, IntersectContext* context)
  {
    AccelN* This = (AccelN*)This_in->ptr;
    for (size_t i=0; i<This->validAccels.size(); i++)
      This->validAccels[i]->intersectors.intersectN(ray,N,context);
  }

  void AccelN::occluded (Accel::Intersectors* This_in, RTCRay& ray, IntersectContext* context) 
  {
    AccelN* This = (AccelN*)This_in->ptr;
    for (size_t i=0; i<This->validAccels.size(); i++) {
      This->validAccels[i]->intersectors.occluded(ray,context); 
      if (ray.tfar < 0.0f) break; 
    }
  }

  void AccelN::occluded4 (const void* valid, Accel::Intersectors* This_in, RTCRay4& ray, IntersectContext* context) 
  {
    AccelN* This = (AccelN*)This_in->ptr;
    for (size_t i=0; i<This->validAccels.size(); i++) {
      This->validAccels[i]->intersectors.occluded4(valid,ray,context);
#if defined(__SSE2__)
      vbool4 valid0 = ((vbool4*)valid)[0];
      vbool4 hit0   = ((vfloat4*)ray.tfar)[0] >= vfloat4(zero);
      if (unlikely(none(valid0 & hit0))) break;
#endif
    }
  }

  void AccelN::occluded8 (const void* valid, Accel::Intersectors* This_in, RTCRay8& ray, IntersectContext* context) 
  {
    AccelN* This = (AccelN*)This_in->ptr;
    for (size_t i=0; i<This->validAccels.size(); i++) {
      This->validAccels[i]->intersectors.occluded8(valid,ray,context);
#if defined(__SSE2__) // FIXME: use higher ISA
      vbool4 valid0 = ((vbool4*)valid)[0];
      vbool4 hit0   = ((vfloat4*)ray.tfar)[0] >= vfloat4(zero);
      vbool4 valid1 = ((vbool4*)valid)[1];
      vbool4 hit1   = ((vfloat4*)ray.tfar)[1] >= vfloat4(zero);
      if (unlikely((none((valid0 & hit0) | (valid1 & hit1))))) break;
#endif
    }
  }

  void AccelN::occluded16 (const void* valid, Accel::Intersectors* This_in, RTCRay16& ray, IntersectContext* context) 
  {
    AccelN* This = (AccelN*)This_in->ptr;
    for (size_t i=0; i<This->validAccels.size(); i++) {
      This->validAccels[i]->intersectors.occluded16(valid,ray,context);
#if defined(__SSE2__) // FIXME: use higher ISA
      vbool4 valid0 = ((vbool4*)valid)[0];
      vbool4 hit0   = ((vfloat4*)ray.tfar)[0] >= vfloat4(zero);
      vbool4 valid1 = ((vbool4*)valid)[1];
      vbool4 hit1   = ((vfloat4*)ray.tfar)[1] >= vfloat4(zero);
      vbool4 valid2 = ((vbool4*)valid)[2];
      vbool4 hit2   = ((vfloat4*)ray.tfar)[2] >= vfloat4(zero);
      vbool4 valid3 = ((vbool4*)valid)[3];
      vbool4 hit3   = ((vfloat4*)ray.tfar)[3] >= vfloat4(zero);
      if (unlikely((none((valid0 & hit0) | (valid1 & hit1) | (valid2 & hit2) | (valid3 & hit3))))) break;
#endif
    }
  }

  void AccelN::occludedN (Accel::Intersectors* This_in, RayK<VSIZEX>** ray, const size_t N, IntersectContext* context)
  {
    AccelN* This = (AccelN*)This_in->ptr;
    size_t M = N;
    for (size_t i=0; i<This->validAccels.size(); i++)
      This->validAccels[i]->intersectors.occludedN(ray,M,context);
  }

  void AccelN::print(size_t ident)
  {
    for (size_t i=0; i<validAccels.size(); i++)
    {
      for (size_t j=0; j<ident; j++) std::cout << " "; 
      std::cout << "accels[" << i << "]" << std::endl;
      validAccels[i]->intersectors.print(ident+2);
    }
  }

  void AccelN::immutable()
  {
    for (size_t i=0; i<accels.size(); i++)
      accels[i]->immutable();
  }
  
  void AccelN::build () 
  {
    /* build all acceleration structures in parallel */
    parallel_for (accels.size(), [&] (size_t i) { 
        accels[i]->build();
      });

    /* create list of non-empty acceleration structures */
    validAccels.clear();
    bool valid1 = true;
    bool valid4 = true;
    bool valid8 = true;
    bool valid16 = true;
    for (size_t i=0; i<accels.size(); i++) {
      if (accels[i]->bounds.empty()) continue;
      validAccels.push_back(accels[i]);
      valid1 &= (bool) accels[i]->intersectors.intersector1;
      valid4 &= (bool) accels[i]->intersectors.intersector4;
      valid8 &= (bool) accels[i]->intersectors.intersector8;
      valid16 &= (bool) accels[i]->intersectors.intersector16;
    }

    if (validAccels.size() == 1) {
      intersectors = validAccels[0]->intersectors;
    }
    else 
    {
      intersectors.ptr = this;
      intersectors.intersector1  = Intersector1(&intersect,&occluded,valid1 ? "AccelN::intersector1": nullptr);
      intersectors.intersector4  = Intersector4(&intersect4,&occluded4,valid4 ? "AccelN::intersector4" : nullptr);
      intersectors.intersector8  = Intersector8(&intersect8,&occluded8,valid8 ? "AccelN::intersector8" : nullptr);
      intersectors.intersector16 = Intersector16(&intersect16,&occluded16,valid16 ? "AccelN::intersector16": nullptr);
      intersectors.intersectorN  = IntersectorN(&intersectN,&occludedN,"AccelN::intersectorN");
    }
    
    /*! calculate bounds */
    bounds = empty;
    for (size_t i=0; i<validAccels.size(); i++) 
      bounds.extend(validAccels[i]->bounds);
  }

  void AccelN::select(bool filter)
  {
    for (size_t i=0; i<accels.size(); i++) 
      accels[i]->intersectors.select(filter);
  }

  void AccelN::deleteGeometry(size_t geomID) 
  {
    for (size_t i=0; i<accels.size(); i++) 
      accels[i]->deleteGeometry(geomID);
  }

  void AccelN::clear()
  {
    for (size_t i=0; i<accels.size(); i++) {
      accels[i]->clear();
    }
  }
}

