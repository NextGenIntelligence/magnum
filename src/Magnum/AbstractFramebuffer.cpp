/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "AbstractFramebuffer.h"

#ifndef MAGNUM_TARGET_GLES2
#include "Magnum/BufferImage.h"
#endif
#include "Magnum/Context.h"
#include "Magnum/Extensions.h"
#include "Magnum/Image.h"

#include "Implementation/FramebufferState.h"
#include "Implementation/State.h"

namespace Magnum {

Vector2i AbstractFramebuffer::maxViewportSize() {
    Vector2i& value = Context::current()->state().framebuffer->maxViewportSize;

    /* Get the value, if not already cached */
    if(value == Vector2i())
        glGetIntegerv(GL_MAX_VIEWPORT_DIMS, value.data());

    return value;
}

Int AbstractFramebuffer::maxDrawBuffers() {
    #ifdef MAGNUM_TARGET_GLES2
    if(!Context::current()->isExtensionSupported<Extensions::GL::NV::draw_buffers>())
        return 0;
    #endif

    GLint& value = Context::current()->state().framebuffer->maxDrawBuffers;

    /* Get the value, if not already cached */
    if(value == 0) {
        #ifndef MAGNUM_TARGET_GLES2
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &value);
        #else
        glGetIntegerv(GL_MAX_DRAW_BUFFERS_NV, &value);
        #endif
    }

    return value;
}

#ifndef MAGNUM_TARGET_GLES
Int AbstractFramebuffer::maxDualSourceDrawBuffers() {
    if(!Context::current()->isExtensionSupported<Extensions::GL::ARB::blend_func_extended>())
        return 0;

    GLint& value = Context::current()->state().framebuffer->maxDualSourceDrawBuffers;

    /* Get the value, if not already cached */
    if(value == 0)
        glGetIntegerv(GL_MAX_DUAL_SOURCE_DRAW_BUFFERS, &value);

    return value;
}
#endif

void AbstractFramebuffer::createIfNotAlready() {
    if(_created) return;

    /* glGen*() does not create the object, just reserves the name. Some
       commands (such as glObjectLabel()) operate with IDs directly and they
       require the object to be created. Binding the framebuffer finally
       creates it. Also all EXT DSA functions implicitly create it. */
    bindInternal();
    CORRADE_INTERNAL_ASSERT(_created);
}

void AbstractFramebuffer::bind() {
    bindInternal(FramebufferTarget::Draw);
    setViewportInternal();
}

void AbstractFramebuffer::bindInternal(FramebufferTarget target) {
    #ifndef MAGNUM_TARGET_GLES2
    bindImplementationDefault(target);
    #else
    (this->*Context::current()->state().framebuffer->bindImplementation)(target);
    #endif
}

#ifdef MAGNUM_TARGET_GLES2
void AbstractFramebuffer::bindImplementationSingle(FramebufferTarget) {
    Implementation::FramebufferState& state = *Context::current()->state().framebuffer;
    CORRADE_INTERNAL_ASSERT(state.readBinding == state.drawBinding);
    if(state.readBinding == _id) return;

    state.readBinding = state.drawBinding = _id;

    /* Binding the framebuffer finally creates it */
    _created = true;
    glBindFramebuffer(GL_FRAMEBUFFER, _id);
}
#endif

#ifndef MAGNUM_TARGET_GLES2
inline
#endif
void AbstractFramebuffer::bindImplementationDefault(FramebufferTarget target) {
    Implementation::FramebufferState& state = *Context::current()->state().framebuffer;

    if(target == FramebufferTarget::Read) {
        if(state.readBinding == _id) return;
        state.readBinding = _id;
    } else if(target == FramebufferTarget::Draw) {
        if(state.drawBinding == _id) return;
        state.drawBinding = _id;
    } else CORRADE_ASSERT_UNREACHABLE();

    /* Binding the framebuffer finally creates it */
    _created = true;
    glBindFramebuffer(GLenum(target), _id);
}

FramebufferTarget AbstractFramebuffer::bindInternal() {
    #ifndef MAGNUM_TARGET_GLES2
    return bindImplementationDefault();
    #else
    return (this->*Context::current()->state().framebuffer->bindInternalImplementation)();
    #endif
}

#ifdef MAGNUM_TARGET_GLES2
FramebufferTarget AbstractFramebuffer::bindImplementationSingle() {
    Implementation::FramebufferState& state = *Context::current()->state().framebuffer;
    CORRADE_INTERNAL_ASSERT(state.readBinding == state.drawBinding);

    /* Bind the framebuffer, if not already */
    if(state.readBinding != _id) {
        state.readBinding = state.drawBinding = _id;

        /* Binding the framebuffer finally creates it */
        _created = true;
        glBindFramebuffer(GL_FRAMEBUFFER, _id);
    }

    return FramebufferTarget{};
}
#endif

#ifndef MAGNUM_TARGET_GLES2
inline
#endif
FramebufferTarget AbstractFramebuffer::bindImplementationDefault() {
    Implementation::FramebufferState& state = *Context::current()->state().framebuffer;

    /* Return target to which the framebuffer is already bound */
    if(state.readBinding == _id)
        return FramebufferTarget::Read;
    if(state.drawBinding == _id)
        return FramebufferTarget::Draw;

    /* Or bind it, if not already */
    state.readBinding = _id;

    /* Binding the framebuffer finally creates it */
    _created = true;
    glBindFramebuffer(GLenum(FramebufferTarget::Read), _id);
    return FramebufferTarget::Read;
}

void AbstractFramebuffer::blit(AbstractFramebuffer& source, AbstractFramebuffer& destination, const Range2Di& sourceRectangle, const Range2Di& destinationRectangle, const FramebufferBlitMask mask, const FramebufferBlitFilter filter) {
    Context::current()->state().framebuffer->blitImplementation(source, destination, sourceRectangle, destinationRectangle, mask, filter);
}

#ifndef MAGNUM_TARGET_GLES2
void AbstractFramebuffer::blitImplementationDefault(AbstractFramebuffer& source, AbstractFramebuffer& destination, const Range2Di& sourceRectangle, const Range2Di& destinationRectangle, const FramebufferBlitMask mask, const FramebufferBlitFilter filter) {
    source.bindInternal(FramebufferTarget::Read);
    destination.bindInternal(FramebufferTarget::Draw);
    glBlitFramebuffer(sourceRectangle.left(), sourceRectangle.bottom(), sourceRectangle.right(), sourceRectangle.top(), destinationRectangle.left(), destinationRectangle.bottom(), destinationRectangle.right(), destinationRectangle.top(), GLbitfield(mask), GLenum(filter));
}

#ifndef MAGNUM_TARGET_GLES
void AbstractFramebuffer::blitImplementationDSA(AbstractFramebuffer& source, AbstractFramebuffer& destination, const Range2Di& sourceRectangle, const Range2Di& destinationRectangle, const FramebufferBlitMask mask, const FramebufferBlitFilter filter) {
    glBlitNamedFramebuffer(source._id, destination._id, sourceRectangle.left(), sourceRectangle.bottom(), sourceRectangle.right(), sourceRectangle.top(), destinationRectangle.left(), destinationRectangle.bottom(), destinationRectangle.right(), destinationRectangle.top(), GLbitfield(mask), GLenum(filter));
}
#endif

#else
void AbstractFramebuffer::blitImplementationANGLE(AbstractFramebuffer& source, AbstractFramebuffer& destination, const Range2Di& sourceRectangle, const Range2Di& destinationRectangle, const FramebufferBlitMask mask, const FramebufferBlitFilter filter) {
    #if !defined(CORRADE_TARGET_EMSCRIPTEN) && !defined(CORRADE_TARGET_NACL)
    source.bindInternal(FramebufferTarget::Read);
    destination.bindInternal(FramebufferTarget::Draw);
    glBlitFramebufferANGLE(sourceRectangle.left(), sourceRectangle.bottom(), sourceRectangle.right(), sourceRectangle.top(), destinationRectangle.left(), destinationRectangle.bottom(), destinationRectangle.right(), destinationRectangle.top(), GLbitfield(mask), GLenum(filter));
    #else
    static_cast<void>(source);
    static_cast<void>(destination);
    static_cast<void>(sourceRectangle);
    static_cast<void>(destinationRectangle);
    static_cast<void>(mask);
    static_cast<void>(filter);
    CORRADE_ASSERT_UNREACHABLE();
    #endif
}

void AbstractFramebuffer::blitImplementationNV(AbstractFramebuffer& source, AbstractFramebuffer& destination, const Range2Di& sourceRectangle, const Range2Di& destinationRectangle, const FramebufferBlitMask mask, const FramebufferBlitFilter filter) {
    #if !defined(CORRADE_TARGET_EMSCRIPTEN) && !defined(CORRADE_TARGET_NACL)
    source.bindInternal(FramebufferTarget::Read);
    destination.bindInternal(FramebufferTarget::Draw);
    glBlitFramebufferNV(sourceRectangle.left(), sourceRectangle.bottom(), sourceRectangle.right(), sourceRectangle.top(), destinationRectangle.left(), destinationRectangle.bottom(), destinationRectangle.right(), destinationRectangle.top(), GLbitfield(mask), GLenum(filter));
    #else
    static_cast<void>(source);
    static_cast<void>(destination);
    static_cast<void>(sourceRectangle);
    static_cast<void>(destinationRectangle);
    static_cast<void>(mask);
    static_cast<void>(filter);
    CORRADE_ASSERT_UNREACHABLE();
    #endif
}
#endif

AbstractFramebuffer& AbstractFramebuffer::setViewport(const Range2Di& rectangle) {
    CORRADE_INTERNAL_ASSERT(rectangle != Implementation::FramebufferState::DisengagedViewport);
    _viewport = rectangle;

    /* Update the viewport if the framebuffer is currently bound */
    if(Context::current()->state().framebuffer->drawBinding == _id)
        setViewportInternal();

    return *this;
}

void AbstractFramebuffer::setViewportInternal() {
    Implementation::FramebufferState& state = *Context::current()->state().framebuffer;

    CORRADE_INTERNAL_ASSERT(_viewport != Implementation::FramebufferState::DisengagedViewport);
    CORRADE_INTERNAL_ASSERT(state.drawBinding == _id);

    /* Already up-to-date, nothing to do */
    if(state.viewport == _viewport)
        return;

    /* Update the state and viewport */
    state.viewport = _viewport;
    glViewport(_viewport.left(), _viewport.bottom(), _viewport.sizeX(), _viewport.sizeY());
}

AbstractFramebuffer& AbstractFramebuffer::clear(const FramebufferClearMask mask) {
    bindInternal(FramebufferTarget::Draw);
    glClear(GLbitfield(mask));

    return *this;
}

void AbstractFramebuffer::read(const Range2Di& rectangle, Image2D& image) {
    bindInternal(FramebufferTarget::Read);
    const std::size_t dataSize = image.dataSize(rectangle.size());
    char* const data = new char[dataSize];
    (Context::current()->state().framebuffer->readImplementation)(rectangle, image.format(), image.type(), dataSize, data);
    image.setData(image.format(), image.type(), rectangle.size(), data);
}

Image2D AbstractFramebuffer::read(const Range2Di& rectangle, Image2D&& image) {
    read(rectangle, image);
    return std::move(image);
}

#ifndef MAGNUM_TARGET_GLES2
void AbstractFramebuffer::read(const Range2Di& rectangle, BufferImage2D& image, BufferUsage usage) {
    bindInternal(FramebufferTarget::Read);
    /* If the buffer doesn't have sufficient size, resize it */
    /** @todo Explicitly reset also when buffer usage changes */
    if(image.size() != rectangle.size())
        image.setData(image.format(), image.type(), rectangle.size(), nullptr, usage);

    image.buffer().bindInternal(Buffer::TargetHint::PixelPack);
    (Context::current()->state().framebuffer->readImplementation)(rectangle, image.format(), image.type(), image.dataSize(rectangle.size()), nullptr);
}

BufferImage2D AbstractFramebuffer::read(const Range2Di& rectangle, BufferImage2D&& image, BufferUsage usage) {
    read(rectangle, image, usage);
    return std::move(image);
}
#endif

void AbstractFramebuffer::invalidateImplementationNoOp(GLsizei, const GLenum* const) {}

void AbstractFramebuffer::invalidateImplementationDefault(const GLsizei count, const GLenum* const attachments) {
    #ifndef MAGNUM_TARGET_GLES2
    glInvalidateFramebuffer(GLenum(bindInternal()), count, attachments);
    #elif !defined(CORRADE_TARGET_EMSCRIPTEN) && !defined(CORRADE_TARGET_NACL)
    glDiscardFramebufferEXT(GLenum(bindInternal()), count, attachments);
    #else
    static_cast<void>(count);
    static_cast<void>(attachments);
    CORRADE_ASSERT_UNREACHABLE();
    #endif
}

#ifndef MAGNUM_TARGET_GLES
void AbstractFramebuffer::invalidateImplementationDSA(const GLsizei count, const GLenum* const attachments) {
    glInvalidateNamedFramebufferData(_id, count, attachments);
}
#endif

#ifndef MAGNUM_TARGET_GLES2
void AbstractFramebuffer::invalidateImplementationNoOp(GLsizei, const GLenum*, const Range2Di&) {}

void AbstractFramebuffer::invalidateImplementationDefault(const GLsizei count, const GLenum* const attachments, const Range2Di& rectangle) {
    glInvalidateSubFramebuffer(GLenum(bindInternal()), count, attachments, rectangle.left(), rectangle.bottom(), rectangle.sizeX(), rectangle.sizeY());
}

#ifndef MAGNUM_TARGET_GLES
void AbstractFramebuffer::invalidateImplementationDSA(const GLsizei count, const GLenum* const attachments, const Range2Di& rectangle) {
    glInvalidateNamedFramebufferSubData(_id, count, attachments, rectangle.left(), rectangle.bottom(), rectangle.sizeX(), rectangle.sizeY());
}
#endif
#endif

GLenum AbstractFramebuffer::checkStatusImplementationDefault(const FramebufferTarget target) {
    bindInternal(target);
    return glCheckFramebufferStatus(GLenum(target));
}

#ifdef MAGNUM_TARGET_GLES2
GLenum AbstractFramebuffer::checkStatusImplementationSingle(FramebufferTarget) {
    bindInternal(FramebufferTarget{});
    return glCheckFramebufferStatus(GL_FRAMEBUFFER);
}
#endif

#ifndef MAGNUM_TARGET_GLES
GLenum AbstractFramebuffer::checkStatusImplementationDSA(const FramebufferTarget target) {
    return glCheckNamedFramebufferStatus(_id, GLenum(target));
}

GLenum AbstractFramebuffer::checkStatusImplementationDSAEXT(const FramebufferTarget target) {
    _created = true;
    return glCheckNamedFramebufferStatusEXT(_id, GLenum(target));
}
#endif

void AbstractFramebuffer::drawBuffersImplementationDefault(GLsizei count, const GLenum* buffers) {
    bindInternal(FramebufferTarget::Draw);

    #ifndef MAGNUM_TARGET_GLES2
    glDrawBuffers(count, buffers);
    #elif !defined(CORRADE_TARGET_EMSCRIPTEN) && !defined(CORRADE_TARGET_NACL)
    glDrawBuffersNV(count, buffers);
    #else
    static_cast<void>(count);
    static_cast<void>(buffers);
    CORRADE_ASSERT_UNREACHABLE();
    #endif
}

#ifndef MAGNUM_TARGET_GLES
void AbstractFramebuffer::drawBuffersImplementationDSA(const GLsizei count, const GLenum* const buffers) {
    glNamedFramebufferDrawBuffers(_id, count, buffers);
}

void AbstractFramebuffer::drawBuffersImplementationDSAEXT(GLsizei count, const GLenum* buffers) {
    _created = true;
    glFramebufferDrawBuffersEXT(_id, count, buffers);
}
#endif

void AbstractFramebuffer::drawBufferImplementationDefault(GLenum buffer) {
    bindInternal(FramebufferTarget::Draw);

    #ifndef MAGNUM_TARGET_GLES
    glDrawBuffer(buffer);
    #elif !defined(MAGNUM_TARGET_GLES2)
    glDrawBuffers(1, &buffer);
    #elif !defined(CORRADE_TARGET_EMSCRIPTEN) && !defined(CORRADE_TARGET_NACL)
    glDrawBuffersNV(1, &buffer);
    #else
    static_cast<void>(buffer);
    CORRADE_ASSERT_UNREACHABLE();
    #endif
}

#ifndef MAGNUM_TARGET_GLES
void AbstractFramebuffer::drawBufferImplementationDSA(const GLenum buffer) {
    glNamedFramebufferDrawBuffer(_id, buffer);
}

void AbstractFramebuffer::drawBufferImplementationDSAEXT(GLenum buffer) {
    _created = true;
    glFramebufferDrawBufferEXT(_id, buffer);
}
#endif

void AbstractFramebuffer::readBufferImplementationDefault(GLenum buffer) {
    bindInternal(FramebufferTarget::Read);

    #ifndef MAGNUM_TARGET_GLES2
    glReadBuffer(buffer);
    #elif !defined(CORRADE_TARGET_EMSCRIPTEN) && !defined(CORRADE_TARGET_NACL)
    glReadBufferNV(buffer);
    #else
    static_cast<void>(buffer);
    CORRADE_ASSERT_UNREACHABLE();
    #endif
}

#ifndef MAGNUM_TARGET_GLES
void AbstractFramebuffer::readBufferImplementationDSA(const GLenum buffer) {
    glFramebufferReadBufferEXT(_id, buffer);
}

void AbstractFramebuffer::readBufferImplementationDSAEXT(GLenum buffer) {
    _created = true;
    glFramebufferReadBufferEXT(_id, buffer);
}
#endif

void AbstractFramebuffer::readImplementationDefault(const Range2Di& rectangle, const ColorFormat format, const ColorType type, const std::size_t, GLvoid* const data) {
    glReadPixels(rectangle.min().x(), rectangle.min().y(), rectangle.sizeX(), rectangle.sizeY(), GLenum(format), GLenum(type), data);
}

void AbstractFramebuffer::readImplementationRobustness(const Range2Di& rectangle, const ColorFormat format, const ColorType type, const std::size_t dataSize, GLvoid* const data) {
    #ifndef MAGNUM_TARGET_GLES
    glReadnPixelsARB(rectangle.min().x(), rectangle.min().y(), rectangle.sizeX(), rectangle.sizeY(), GLenum(format), GLenum(type), dataSize, data);
    #elif !defined(CORRADE_TARGET_EMSCRIPTEN) && !defined(CORRADE_TARGET_NACL)
    glReadnPixelsEXT(rectangle.min().x(), rectangle.min().y(), rectangle.sizeX(), rectangle.sizeY(), GLenum(format), GLenum(type), dataSize, data);
    #else
    static_cast<void>(rectangle);
    static_cast<void>(format);
    static_cast<void>(type);
    static_cast<void>(dataSize);
    static_cast<void>(data);
    CORRADE_ASSERT_UNREACHABLE();
    #endif
}

}
