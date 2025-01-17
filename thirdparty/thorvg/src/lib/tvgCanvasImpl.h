/*
 * Copyright (c) 2020-2021 Samsung Electronics Co., Ltd. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef _TVG_CANVAS_IMPL_H_
#define _TVG_CANVAS_IMPL_H_

#include "tvgPaint.h"

/************************************************************************/
/* Internal Class Implementation                                        */
/************************************************************************/

struct Canvas::Impl
{
    Array<Paint*> paints;
    RenderMethod* renderer;
    bool refresh = false;   //if all paints should be updated by force.
    bool drawing = false;   //on drawing condition?

    Impl(RenderMethod* pRenderer):renderer(pRenderer)
    {
    }

    ~Impl()
    {
        clear(true);
        delete(renderer);
    }

    Result push(unique_ptr<Paint> paint)
    {
        //You can not push paints during rendering.
        if (drawing) return Result::InsufficientCondition;

        auto p = paint.release();
        if (!p) return Result::MemoryCorruption;
        paints.push(p);

        return update(p, true);
    }

    Result clear(bool free)
    {
        //Clear render target before drawing
        if (!renderer || !renderer->clear()) return Result::InsufficientCondition;

        //Free paints
        for (auto paint = paints.data; paint < (paints.data + paints.count); ++paint) {
            (*paint)->pImpl->dispose(*renderer);
            if (free) delete(*paint);
        }

        paints.clear();

        drawing = false;

        return Result::Success;
    }

    void needRefresh()
    {
        refresh = true;
    }

    Result update(Paint* paint, bool force)
    {
        if (paints.count == 0 || drawing || !renderer) return Result::InsufficientCondition;

        Array<RenderData> clips;
        auto flag = RenderUpdateFlag::None;
        if (refresh || force) flag = RenderUpdateFlag::All;

        //Update single paint node
        if (paint) {
            //Optimize Me: Can we skip the searching?
            for (auto paint2 = paints.data; paint2 < (paints.data + paints.count); ++paint2) {
                if ((*paint2) == paint) {
                    paint->pImpl->update(*renderer, nullptr, 255, clips, flag);
                    return Result::Success;
                }
            }
            return Result::InvalidArguments;
        //Update all retained paint nodes
        } else {
            for (auto paint = paints.data; paint < (paints.data + paints.count); ++paint) {
                (*paint)->pImpl->update(*renderer, nullptr, 255, clips, flag);
            }
        }

        refresh = false;

        return Result::Success;
    }

    Result draw()
    {
        if (drawing || paints.count == 0 || !renderer || !renderer->preRender()) return Result::InsufficientCondition;

        bool rendered = false;
        for (auto paint = paints.data; paint < (paints.data + paints.count); ++paint) {
            if ((*paint)->pImpl->render(*renderer)) rendered = true;
        }

        if (!rendered || !renderer->postRender()) return Result::InsufficientCondition;

        drawing = true;

        return Result::Success;
    }

    Result sync()
    {
        if (!drawing) return Result::InsufficientCondition;

        if (renderer->sync()) {
            drawing = false;
            return Result::Success;
        }

        return Result::InsufficientCondition;
    }
};

#endif /* _TVG_CANVAS_IMPL_H_ */
