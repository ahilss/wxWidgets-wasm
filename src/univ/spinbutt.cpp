///////////////////////////////////////////////////////////////////////////////
// Name:        src/univ/spinbutt.cpp
// Purpose:     implementation of the universal version of wxSpinButton
// Author:      Vadim Zeitlin
// Modified by:
// Created:     21.01.01
// Copyright:   (c) 2001 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"


#ifndef WX_PRECOMP
#endif

#include "wx/spinbutt.h"

#if wxUSE_SPINBTN

#include "wx/univ/renderer.h"
#include "wx/univ/inphand.h"
#include "wx/univ/theme.h"

// ============================================================================
// implementation of wxSpinButton
// ============================================================================

// ----------------------------------------------------------------------------
// creation
// ----------------------------------------------------------------------------

#ifdef __VISUALC__
    // warning C4355: 'this' : used in base member initializer list
    #pragma warning(disable:4355)  // so what? disable it...
#endif

wxSpinButton::wxSpinButton()
            : m_arrows(this)
{
    Init();
}

wxSpinButton::wxSpinButton(wxWindow *parent,
                           wxWindowID id,
                           const wxPoint& pos,
                           const wxSize& size,
                           long style,
                           const wxString& name)
            : m_arrows(this)
{
    Init();

    (void)Create(parent, id, pos, size, style, name);
}

#ifdef __VISUALC__
    // warning C4355: 'this' : used in base member initializer list
    #pragma warning(default:4355)
#endif

void wxSpinButton::Init()
{
    for ( size_t n = 0; n < WXSIZEOF(m_arrowsState); n++ )
    {
        m_arrowsState[n] = 0;
    }

    m_value = 0;
}

bool wxSpinButton::Create(wxWindow *parent,
                          wxWindowID id,
                          const wxPoint& pos,
                          const wxSize& size,
                          long style,
                          const wxString& name)
{
    // the spin buttons never have the border
    style &= ~wxBORDER_MASK;

    if ( !wxSpinButtonBase::Create(parent, id, pos, size, style,
                                   wxDefaultValidator, name) )
        return false;

    SetInitialSize(size);

    CreateInputHandler(wxINP_HANDLER_SPINBTN);

    return true;
}

// ----------------------------------------------------------------------------
// value access
// ----------------------------------------------------------------------------

void wxSpinButton::SetRange(int minVal, int maxVal)
{
    wxSpinButtonBase::SetRange(minVal, maxVal);

    // because the arrows disabled state might have changed - we don't check if
    // it really changed or not because SetRange() is called rarely enough and
    // son an extre refresh here doesn't really hurt
    Refresh();
}

int wxSpinButton::GetValue() const
{
    return m_value;
}

void wxSpinButton::SetValue(int val)
{
    if ( val != m_value )
    {
        m_value = val;

        Refresh();
    }
}

int wxSpinButton::NormalizeValue(int value) const
{
    if ( value > m_max )
    {
        if ( GetWindowStyleFlag() & wxSP_WRAP )
            value = m_min + (value - m_max - 1) % (m_max - m_min + 1);
        else
            value = m_max;
    }
    else if ( value < m_min )
    {
        if ( GetWindowStyleFlag() & wxSP_WRAP )
            value = m_max - (m_min - value - 1) % (m_max - m_min + 1);
        else
            value = m_min;
    }

    return value;
}

bool wxSpinButton::ChangeValue(int inc)
{
    int valueNew = NormalizeValue(m_value + inc);

    if ( valueNew == m_value )
    {
        // nothing changed - most likely because we are already at min/max
        // value
        return false;
    }

    wxSpinEvent event(inc > 0 ? wxEVT_SCROLL_LINEUP : wxEVT_SCROLL_LINEDOWN,
                      GetId());
    event.SetPosition(valueNew);
    event.SetEventObject(this);

    if ( GetEventHandler()->ProcessEvent(event) && !event.IsAllowed() )
    {
        // programm has vetoed the event
        return false;
    }

    m_value = valueNew;

    // send wxEVT_SCROLL_THUMBTRACK as well
    event.SetEventType(wxEVT_SCROLL_THUMBTRACK);
    (void)GetEventHandler()->ProcessEvent(event);

    return true;
}

// ----------------------------------------------------------------------------
// size calculations
// ----------------------------------------------------------------------------

wxSize wxSpinButton::DoGetBestClientSize() const
{
    // a spin button has by default the same size as two scrollbar arrows put
    // together
    wxSize size;
    if ( IsVertical() )
    {
        size = m_renderer->GetScrollbarArrowSize(wxVERTICAL);
        size.y *= 2;
    }
    else
    {
        size = m_renderer->GetScrollbarArrowSize(wxHORIZONTAL);
        size.x *= 2;
    }

    return size;
}

// ----------------------------------------------------------------------------
// wxControlWithArrows methods
// ----------------------------------------------------------------------------

int wxSpinButton::GetArrowState(wxScrollArrows::Arrow arrow) const
{
    int state = m_arrowsState[arrow];

    // the arrow may also be disabled: either because the control is completely
    // disabled
    bool disabled = !IsEnabled();

    if ( !disabled && !(GetWindowStyleFlag() & wxSP_WRAP) )
    {
        // ... or because we can't go any further - note that this never
        // happens if we just wrap
        if ( IsVertical() )
        {
            if ( arrow == wxScrollArrows::Arrow_First )
                disabled = m_value == m_max;
            else
                disabled = m_value == m_min;
        }
        else // horizontal
        {
            if ( arrow == wxScrollArrows::Arrow_First )
                disabled = m_value == m_min;
            else
                disabled = m_value == m_max;
        }
    }

    if ( disabled )
    {
        state |= wxCONTROL_DISABLED;
    }

    return state;
}

void wxSpinButton::SetArrowFlag(wxScrollArrows::Arrow arrow, int flag, bool set)
{
    int state = m_arrowsState[arrow];
    if ( set )
        state |= flag;
    else
        state &= ~flag;

    if ( state != m_arrowsState[arrow] )
    {
        m_arrowsState[arrow] = state;
        Refresh();
    }
}

bool wxSpinButton::OnArrow(wxScrollArrows::Arrow arrow)
{
    int valueOld = GetValue();

    wxControlAction action;
    if ( arrow == wxScrollArrows::Arrow_First )
        action = IsVertical() ? wxACTION_SPIN_INC : wxACTION_SPIN_DEC;
    else
        action = IsVertical() ? wxACTION_SPIN_DEC : wxACTION_SPIN_INC;

    PerformAction(action);

    // did we scroll to the end?
    return GetValue() != valueOld;
}

// ----------------------------------------------------------------------------
// drawing
// ----------------------------------------------------------------------------

void wxSpinButton::DoDraw(wxControlRenderer *renderer)
{
    wxRect rectArrow1, rectArrow2;
    CalcArrowRects(&rectArrow1, &rectArrow2);

    wxDC& dc = renderer->GetDC();
    m_arrows.DrawArrow(wxScrollArrows::Arrow_First, dc, rectArrow1);
    m_arrows.DrawArrow(wxScrollArrows::Arrow_Second, dc, rectArrow2);
}

// ----------------------------------------------------------------------------
// geometry
// ----------------------------------------------------------------------------

void wxSpinButton::CalcArrowRects(wxRect *rect1, wxRect *rect2) const
{
    const wxCoord ARROW_WIDTH = 9;
    const wxCoord ARROW_HEIGHT = 5;

    wxRect rectTotal = GetClientRect();

    if ( IsVertical() )
    {
        wxCoord h = rectTotal.height / 2;
        wxCoord w = rectTotal.width;

        rect1->x = rectTotal.x + (w - ARROW_WIDTH) / 2;
        rect1->y = rectTotal.y + (h - ARROW_HEIGHT) / 2;
        rect1->width = ARROW_WIDTH;
        rect1->height = ARROW_HEIGHT;

        rect2->x = rect1->x;
        rect2->y = rect1->y + h;
        rect2->width = ARROW_WIDTH;
        rect2->height = ARROW_HEIGHT;
    }
    else // horizontal
    {
        wxCoord h = rectTotal.height;
        wxCoord w = rectTotal.width / 2;

        rect1->x = rectTotal.x + (w - ARROW_WIDTH) / 2;
        rect1->y = rectTotal.y + (h - ARROW_HEIGHT) / 2;
        rect1->width = ARROW_WIDTH;
        rect1->height = ARROW_HEIGHT;

        rect2->x = rect1->x + w;
        rect2->y = rect1->y;
        rect2->width = ARROW_WIDTH;
        rect2->height = ARROW_HEIGHT;
    }
}

wxScrollArrows::Arrow wxSpinButton::HitTestArrow(const wxPoint& pt) const
{
    wxRect rectArrow1, rectArrow2;
    CalcArrowRects(&rectArrow1, &rectArrow2);

    if ( rectArrow1.Contains(pt) )
        return wxScrollArrows::Arrow_First;
    else if ( rectArrow2.Contains(pt) )
        return wxScrollArrows::Arrow_Second;
    else
        return wxScrollArrows::Arrow_None;
}

// ----------------------------------------------------------------------------
// input processing
// ----------------------------------------------------------------------------

bool wxSpinButton::PerformAction(const wxControlAction& action,
                                 long numArg,
                                 const wxString& strArg)
{
    if ( action == wxACTION_SPIN_INC )
        ChangeValue(+1);
    else if ( action == wxACTION_SPIN_DEC )
        ChangeValue(-1);
    else
        return wxControl::PerformAction(action, numArg, strArg);

    return true;
}

/* static */
wxInputHandler *wxSpinButton::GetStdInputHandler(wxInputHandler *handlerDef)
{
    static wxStdSpinButtonInputHandler s_handler(handlerDef);

    return &s_handler;
}

// ----------------------------------------------------------------------------
// wxStdSpinButtonInputHandler
// ----------------------------------------------------------------------------

wxStdSpinButtonInputHandler::
wxStdSpinButtonInputHandler(wxInputHandler *inphand)
    : wxStdInputHandler(inphand)
{
}

bool wxStdSpinButtonInputHandler::HandleKey(wxInputConsumer *consumer,
                                            const wxKeyEvent& event,
                                            bool pressed)
{
    if ( pressed )
    {
        wxControlAction action;
        switch ( event.GetKeyCode() )
        {
            case WXK_DOWN:
            case WXK_RIGHT:
                action = wxACTION_SPIN_DEC;
                break;

            case WXK_UP:
            case WXK_LEFT:
                action = wxACTION_SPIN_INC;
                break;
        }

        if ( !action.IsEmpty() )
        {
            consumer->PerformAction(action);

            return true;
        }
    }

    return wxStdInputHandler::HandleKey(consumer, event, pressed);
}

bool wxStdSpinButtonInputHandler::HandleMouse(wxInputConsumer *consumer,
                                              const wxMouseEvent& event)
{
    wxSpinButton *spinbtn = wxStaticCast(consumer->GetInputWindow(), wxSpinButton);

    if ( spinbtn->GetArrows().HandleMouse(event) )
    {
        // don't refresh, everything is already done
        return false;
    }

    return wxStdInputHandler::HandleMouse(consumer, event);
}

bool wxStdSpinButtonInputHandler::HandleMouseMove(wxInputConsumer *consumer,
                                                  const wxMouseEvent& event)
{
    wxSpinButton *spinbtn = wxStaticCast(consumer->GetInputWindow(), wxSpinButton);

    if ( spinbtn->GetArrows().HandleMouseMove(event) )
    {
        // processed by the arrows
        return false;
    }

    return wxStdInputHandler::HandleMouseMove(consumer, event);
}


#endif // wxUSE_SPINBTN
