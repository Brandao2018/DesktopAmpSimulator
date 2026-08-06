#pragma once

// Hardware look for a CustomKnob. Split out from CustomKnob.h (which pulls
// in QWidget) so pure-data consumers — like AmpCatalog.h — don't drag the
// whole Qt Widgets module into headless contexts (e.g. unit tests linking
// only Qt Gui for QColor).

enum class KnobStyle
{
    Studio,          // charcoal cap + orange value arc (rack sections)
    FenderSkirted,   // black skirted knob, numbered 1-10 skirt, white line
    TweedChicken,    // brown chicken-head pointer knob (tweed era)
    MarshallGold,    // knurled gold top-hat with black indicator
    MesaMetal,       // machined black/steel knob with white indicator
    OrangeChicken,   // black chicken-head with white nose dot
    Stomp,           // small black fluted pedal knob
};
