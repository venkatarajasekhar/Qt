//
// Copyright 2012 Francisco Jerez
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
// OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#ifndef __CORE_RESOURCE_HPP__
#define __CORE_RESOURCE_HPP__

#include <list>

#include "core/base.hpp"
#include "core/memory.hpp"
#include "core/geometry.hpp"
#include "pipe/p_state.h"

namespace clover {
   class mapping;

   ///
   /// Class that represents a device-specific instance of some memory
   /// object.
   ///
   class resource {
   public:
      typedef clover::point<size_t, 3> point;

      resource(const resource &r) = delete;
      virtual ~resource();

      void copy(command_queue &q, const point &origin, const point &region,
                resource &src_resource, const point &src_origin);

      void *add_map(command_queue &q, cl_map_flags flags, bool blocking,
                    const point &origin, const point &region);
      void del_map(void *p);
      unsigned map_count() const;

      clover::device &dev;
      clover::memory_obj &obj;

      friend class sub_resource;
      friend class mapping;
      friend struct ::_cl_kernel;

   protected:
      resource(clover::device &dev, clover::memory_obj &obj);

      pipe_sampler_view *bind_sampler_view(clover::command_queue &q);
      void unbind_sampler_view(clover::command_queue &q,
                               pipe_sampler_view *st);

      pipe_surface *bind_surface(clover::command_queue &q, bool rw);
      void unbind_surface(clover::command_queue &q, pipe_surface *st);

      pipe_resource *pipe;
      point offset;

   private:
      std::list<mapping> maps;
   };

   ///
   /// Resource associated with its own top-level data storage
   /// allocated in some device.
   ///
   class root_resource : public resource {
   public:
      root_resource(clover::device &dev, clover::memory_obj &obj,
                    clover::command_queue &q, const std::string &data);
      root_resource(clover::device &dev, clover::memory_obj &obj,
                    root_resource &r);
      virtual ~root_resource();
   };

   ///
   /// Resource that reuses a portion of some other resource as data
   /// storage.
   ///
   class sub_resource : public resource {
   public:
      sub_resource(clover::resource &r, point offset);
   };

   ///
   /// Class that represents a mapping of some resource into the CPU
   /// memory space.
   ///
   class mapping {
   public:
      mapping(command_queue &q, resource &r, cl_map_flags flags,
              bool blocking, const resource::point &origin,
              const resource::point &region);
      mapping(const mapping &m) = delete;
      mapping(mapping &&m);
      ~mapping();

      operator void *() {
         return p;
      }

      operator char *() {
         return (char *)p;
      }

   private:
      pipe_context *pctx;
      pipe_transfer *pxfer;
      void *p;
   };
}

#endif
