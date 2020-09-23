#include "idtag.h"

TagConverter::TagIdentifier TagConverter::m_tagIdentifier = {{{"kinder", "children"}, Tag::Children },
                                               {{"deutsch", "german"}, Tag::German },
                                               {{"rock"}, Tag::Rock },
                                               {{"pop"}, Tag::Pop },
                                               {{"jazz"}, Tag::Jazz },
                                               {{"playlist:"}, Tag::Playlist },
                                               {{"radio", "stream"}, Tag::Stream }
                                               };

