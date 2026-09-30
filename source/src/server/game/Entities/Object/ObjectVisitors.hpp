#ifndef LEGIONFORGE_GAME_OBJECT_VISITORS_HPP
#define LEGIONFORGE_GAME_OBJECT_VISITORS_HPP

#include "Object.h"
#include "Map.h"

namespace LegionForge {

template <typename Notifier>
inline void VisitNearbyObject(WorldObject const *obj, float radius, Notifier &notifier)
{
    if (obj->IsInWorld())
        obj->GetMap()->VisitAll(obj->GetPositionX(), obj->GetPositionY(), radius, notifier);
}

template <typename Notifier>
inline void VisitNearbyGridObject(WorldObject const *obj, float radius, Notifier &notifier)
{
    if (obj->IsInWorld())
        obj->GetMap()->VisitGrid(obj->GetPositionX(), obj->GetPositionY(), radius, notifier);
}

template <typename Notifier>
inline void VisitNearbyWorldObject(WorldObject const *obj, float radius, Notifier &notifier)
{
    if (obj->IsInWorld())
    {
        if (Map* map = obj->GetMap())
            map->VisitWorld(obj->GetPositionX(), obj->GetPositionY(), radius, notifier);
    }
}

} // namespace LegionForge

#endif // LEGIONFORGE_GAME_OBJECT_VISITORS_HPP
