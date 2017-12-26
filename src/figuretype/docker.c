#include "docker.h"

#include "building/building.h"
#include "building/storage.h"
#include "building/warehouse.h"
#include "core/calc.h"
#include "empire/city.h"
#include "empire/empire.h"
#include "empire/trade_route.h"
#include "figure/image.h"
#include "figure/route.h"
#include "figure/trader.h"
#include "game/resource.h"
#include "graphics/image.h"
#include "map/road_access.h"

#include "Data/CityInfo.h"
#include "FigureAction.h"
#include "FigureMovement.h"

static int try_import_resource(int building_id, int resource, int city_id)
{
    building *warehouse = building_get(building_id);
    if (warehouse->type != BUILDING_WAREHOUSE) {
        return 0;
    }
    
    int route_id = empire_city_get_route_id(city_id);
    // try existing storage bay with the same resource
    building *space = warehouse;
    for (int i = 0; i < 8; i++) {
        space = building_next(space);
        if (space->id > 0) {
            if (space->loadsStored && space->loadsStored < 4 && space->subtype.warehouseResourceId == resource) {
                trade_route_increase_traded(route_id, resource);
                building_warehouse_space_add_import(space, resource);
                return 1;
            }
        }
    }
    // try unused storage bay
    space = warehouse;
    for (int i = 0; i < 8; i++) {
        space = building_next(space);
        if (space->id > 0) {
            if (space->subtype.warehouseResourceId == RESOURCE_NONE) {
                trade_route_increase_traded(route_id, resource);
                building_warehouse_space_add_import(space, resource);
                return 1;
            }
        }
    }
    return 0;
}

static int try_export_resource(int building_id, int resource, int city_id)
{
    building *warehouse = building_get(building_id);
    if (warehouse->type != BUILDING_WAREHOUSE) {
        return 0;
    }
    
    building *space = warehouse;
    for (int i = 0; i < 8; i++) {
        space = building_next(space);
        if (space->id > 0) {
            if (space->loadsStored && space->subtype.warehouseResourceId == resource) {
                trade_route_increase_traded(empire_city_get_route_id(city_id), resource);
                building_warehouse_space_remove_export(space, resource);
                return 1;
            }
        }
    }
    return 0;
}

static int get_closest_warehouse_for_import(int x, int y, int city_id, int distance_from_entry, int road_network_id,
                                            int *x_warehouse, int *y_warehouse)
{
    int importable[16];
    importable[RESOURCE_NONE] = 0;
    for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
        importable[r] = empire_can_import_resource_from_city(city_id, r);
    }
    Data_CityInfo.tradeNextImportResourceDocker++;
    if (Data_CityInfo.tradeNextImportResourceDocker > 15) {
        Data_CityInfo.tradeNextImportResourceDocker = 1;
    }
    for (int i = RESOURCE_MIN; i < RESOURCE_MAX && !importable[Data_CityInfo.tradeNextImportResourceDocker]; i++) {
        Data_CityInfo.tradeNextImportResourceDocker++;
        if (Data_CityInfo.tradeNextImportResourceDocker > 15) {
            Data_CityInfo.tradeNextImportResourceDocker = 1;
        }
    }
    if (!importable[Data_CityInfo.tradeNextImportResourceDocker]) {
        return 0;
    }
    int min_distance = 10000;
    int min_building_id = 0;
    int resource = Data_CityInfo.tradeNextImportResourceDocker;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        building *b = building_get(i);
        if (!BuildingIsInUse(b) || b->type != BUILDING_WAREHOUSE) {
            continue;
        }
        if (!b->hasRoadAccess || b->distanceFromEntry <= 0) {
            continue;
        }
        if (b->roadNetworkId != road_network_id) {
            continue;
        }
        const building_storage *s = building_storage_get(b->storage_id);
        if (s->resource_state[resource] != BUILDING_STORAGE_STATE_NOT_ACCEPTING && !s->empty_all) {
            int distance_penalty = 32;
            building *space = b;
            for (int s= 0; s < 8; s++) {
                space = building_next(space);
                if (space->id && space->subtype.warehouseResourceId == RESOURCE_NONE) {
                    distance_penalty -= 8;
                }
                if (space->id && space->subtype.warehouseResourceId == resource && space->loadsStored < 4) {
                    distance_penalty -= 4;
                }
            }
            if (distance_penalty < 32) {
                int distance = calc_distance_with_penalty(b->x, b->y, x, y, distance_from_entry, b->distanceFromEntry);
                // prefer emptier warehouse
                distance += distance_penalty;
                if (distance < min_distance) {
                    min_distance = distance;
                    min_building_id = i;
                }
            }
        }
    }
    if (!min_building_id) {
        return 0;
    }
    building *min = building_get(min_building_id);
    if (min->hasRoadAccess == 1) {
        *x_warehouse = min->x;
        *y_warehouse = min->y;
    } else if (!map_has_road_access(min->x, min->y, 3, x_warehouse, y_warehouse)) {
        return 0;
    }
    return min_building_id;
}

static int get_closest_warehouse_for_export(int x, int y, int city_id, int distance_from_entry, int road_network_id,
                                            int *x_warehouse, int *y_warehouse)
{
    int exportable[16];
    exportable[RESOURCE_NONE] = 0;
    for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
        exportable[r] = empire_can_export_resource_to_city(city_id, r);
    }
    Data_CityInfo.tradeNextExportResourceDocker++;
    if (Data_CityInfo.tradeNextExportResourceDocker > 15) {
        Data_CityInfo.tradeNextExportResourceDocker = 1;
    }
    for (int i = RESOURCE_MIN; i < RESOURCE_MAX && !exportable[Data_CityInfo.tradeNextExportResourceDocker]; i++) {
        Data_CityInfo.tradeNextExportResourceDocker++;
        if (Data_CityInfo.tradeNextExportResourceDocker > 15) {
            Data_CityInfo.tradeNextExportResourceDocker = 1;
        }
    }
    if (!exportable[Data_CityInfo.tradeNextExportResourceDocker]) {
        return 0;
    }
    int min_distance = 10000;
    int min_building_id = 0;
    int resource = Data_CityInfo.tradeNextExportResourceDocker;
    
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        building *b = building_get(i);
        if (!BuildingIsInUse(b) || b->type != BUILDING_WAREHOUSE) {
            continue;
        }
        if (!b->hasRoadAccess || b->distanceFromEntry <= 0) {
            continue;
        }
        if (b->roadNetworkId != road_network_id) {
            continue;
        }
        int distance_penalty = 32;
        building *space = b;
        for (int s = 0; s < 8; s++) {
            space = building_next(space);
            if (space->id && space->subtype.warehouseResourceId == resource && space->loadsStored > 0) {
                distance_penalty--;
            }
        }
        if (distance_penalty < 32) {
            int distance = calc_distance_with_penalty(b->x, b->y, x, y, distance_from_entry, b->distanceFromEntry);
            // prefer fuller warehouse
            distance += distance_penalty;
            if (distance < min_distance) {
                min_distance = distance;
                min_building_id = i;
            }
        }
    }
    if (!min_building_id) {
        return 0;
    }
    building *min = building_get(min_building_id);
    if (min->hasRoadAccess == 1) {
        *x_warehouse = min->x;
        *y_warehouse = min->y;
    } else if (!map_has_road_access(min->x, min->y, 3, x_warehouse, y_warehouse)) {
        return 0;
    }
    return min_building_id;
}

static int deliver_import_resource(figure *f, building *dock)
{
    int ship_id = dock->data.other.boatFigureId;
    if (!ship_id) {
        return 0;
    }
    figure *ship = figure_get(ship_id);
    if (ship->actionState != FigureActionState_112_TradeShipMoored || ship->loadsSoldOrCarrying <= 0) {
        return 0;
    }
    int x, y;
    if (Data_CityInfo.buildingTradeCenterBuildingId) {
        building *trade_center = building_get(Data_CityInfo.buildingTradeCenterBuildingId);
        x = trade_center->x;
        y = trade_center->y;
    } else {
        x = f->x;
        y = f->y;
    }
    int x_tile, y_tile;
    int warehouse_id = get_closest_warehouse_for_import(x, y, ship->empireCityId,
                      dock->distanceFromEntry, dock->roadNetworkId, &x_tile, &y_tile);
    if (!warehouse_id) {
        return 0;
    }
    ship->loadsSoldOrCarrying--;
    f->destinationBuildingId = warehouse_id;
    f->waitTicks = 0;
    f->actionState = FigureActionState_133_DockerImportQueue;
    f->destinationX = x_tile;
    f->destinationY = y_tile;
    f->resourceId = Data_CityInfo.tradeNextImportResourceDocker;
    return 1;
}

static int fetch_export_resource(figure *f, building *dock)
{
    int ship_id = dock->data.other.boatFigureId;
    if (!ship_id) {
        return 0;
    }
    figure *ship = figure_get(ship_id);
    if (ship->actionState != FigureActionState_112_TradeShipMoored || ship->traderAmountBought >= 12) {
        return 0;
    }
    int x, y;
    if (Data_CityInfo.buildingTradeCenterBuildingId) {
        building *trade_center = building_get(Data_CityInfo.buildingTradeCenterBuildingId);
        x = trade_center->x;
        y = trade_center->y;
    } else {
        x = f->x;
        y = f->y;
    }
    int x_tile, y_tile;
    int warehouse_id = get_closest_warehouse_for_export(x, y, ship->empireCityId,
        dock->distanceFromEntry, dock->roadNetworkId, &x_tile, &y_tile);
    if (!warehouse_id) {
        return 0;
    }
    ship->traderAmountBought++;
    f->destinationBuildingId = warehouse_id;
    f->actionState = FigureActionState_136_DockerExportGoingToWarehouse;
    f->waitTicks = 0;
    f->destinationX = x_tile;
    f->destinationY = y_tile;
    f->resourceId = Data_CityInfo.tradeNextExportResourceDocker;
    return 1;
}

static void set_cart_graphic(figure *f)
{
    f->cartGraphicId = image_group(GROUP_FIGURE_CARTPUSHER_CART) + 8 * f->resourceId;
    f->cartGraphicId += resource_image_offset(f->resourceId, RESOURCE_IMAGE_CART);
}

void figure_docker_action(figure *f)
{
    building *b = building_get(f->buildingId);
    figure_image_increase_offset(f, 12);
    f->cartGraphicId = 0;
    if (!BuildingIsInUse(b)) {
        f->state = FigureState_Dead;
    }
    if (b->type != BUILDING_DOCK && b->type != BUILDING_WHARF) {
        f->state = FigureState_Dead;
    }
    if (b->data.other.dockNumShips) {
        b->data.other.dockNumShips--;
    }
    if (b->data.other.boatFigureId) {
        figure *ship = figure_get(b->data.other.boatFigureId);
        if (ship->state != FigureState_Alive || ship->type != FIGURE_TRADE_SHIP) {
            b->data.other.boatFigureId = 0;
        } else if (trader_has_traded_max(ship->traderId)) {
            b->data.other.boatFigureId = 0;
        } else if (ship->actionState == FigureActionState_115_TradeShipLeaving) {
            b->data.other.boatFigureId = 0;
        }
    }
    f->terrainUsage = FigureTerrainUsage_Roads;
    switch (f->actionState) {
        case FigureActionState_150_Attack:
            FigureAction_Common_handleAttack(f);
            break;
        case FigureActionState_149_Corpse:
            FigureAction_Common_handleCorpse(f);
            break;
        case FigureActionState_132_DockerIdling:
            f->resourceId = 0;
            f->cartGraphicId = 0;
            if (!deliver_import_resource(f, b)) {
                fetch_export_resource(f, b);
            }
            f->graphicOffset = 0;
            break;
        case FigureActionState_133_DockerImportQueue:
            f->cartGraphicId = 0;
            f->graphicOffset = 0;
            if (b->data.other.dockQueuedDockerId <= 0) {
                b->data.other.dockQueuedDockerId = f->id;
                f->waitTicks = 0;
            }
            if (b->data.other.dockQueuedDockerId == f->id) {
                b->data.other.dockNumShips = 120;
                f->waitTicks++;
                if (f->waitTicks >= 80) {
                    f->actionState = FigureActionState_135_DockerImportGoingToWarehouse;
                    f->waitTicks = 0;
                    set_cart_graphic(f);
                    b->data.other.dockQueuedDockerId = 0;
                }
            } else {
                int has_queued_docker = 0;
                for (int i = 0; i < 3; i++) {
                    if (b->data.other.dockFigureIds[i]) {
                        figure *docker = figure_get(b->data.other.dockFigureIds[i]);
                        if (docker->id == b->data.other.dockQueuedDockerId && docker->state == FigureState_Alive) {
                            if (docker->actionState == FigureActionState_133_DockerImportQueue ||
                                docker->actionState == FigureActionState_134_DockerExportQueue) {
                                has_queued_docker = 1;
                            }
                        }
                    }
                }
                if (!has_queued_docker) {
                    b->data.other.dockQueuedDockerId = 0;
                }
            }
            break;
        case FigureActionState_134_DockerExportQueue:
            set_cart_graphic(f);
            if (b->data.other.dockQueuedDockerId <= 0) {
                b->data.other.dockQueuedDockerId = f->id;
                f->waitTicks = 0;
            }
            if (b->data.other.dockQueuedDockerId == f->id) {
                b->data.other.dockNumShips = 120;
                f->waitTicks++;
                if (f->waitTicks >= 80) {
                    f->actionState = FigureActionState_132_DockerIdling;
                    f->waitTicks = 0;
                    f->graphicId = 0;
                    f->cartGraphicId = 0;
                    b->data.other.dockQueuedDockerId = 0;
                }
            }
            f->waitTicks++;
            if (f->waitTicks >= 20) {
                f->actionState = FigureActionState_132_DockerIdling;
                f->waitTicks = 0;
            }
            f->graphicOffset = 0;
            break;
        case FigureActionState_135_DockerImportGoingToWarehouse:
            set_cart_graphic(f);
            FigureMovement_walkTicks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->actionState = FigureActionState_139_DockerImportAtWarehouse;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                f->state = FigureState_Dead;
            }
            if (!BuildingIsInUse(building_get(f->destinationBuildingId))) {
                f->state = FigureState_Dead;
            }
            break;
        case FigureActionState_136_DockerExportGoingToWarehouse:
            f->cartGraphicId = image_group(GROUP_FIGURE_CARTPUSHER_CART); // empty
            FigureMovement_walkTicks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->actionState = FigureActionState_140_DockerExportAtWarehouse;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                f->state = FigureState_Dead;
            }
            if (!BuildingIsInUse(building_get(f->destinationBuildingId))) {
                f->state = FigureState_Dead;
            }
            break;
        case FigureActionState_137_DockerExportReturning:
            set_cart_graphic(f);
            FigureMovement_walkTicks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->actionState = FigureActionState_134_DockerExportQueue;
                f->waitTicks = 0;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                f->state = FigureState_Dead;
            }
            if (!BuildingIsInUse(building_get(f->destinationBuildingId))) {
                f->state = FigureState_Dead;
            }
            break;
        case FigureActionState_138_DockerImportReturning:
            set_cart_graphic(f);
            FigureMovement_walkTicks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->actionState = FigureActionState_132_DockerIdling;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                f->state = FigureState_Dead;
            }
            break;
        case FigureActionState_139_DockerImportAtWarehouse:
            set_cart_graphic(f);
            f->waitTicks++;
            if (f->waitTicks > 10) {
                int trade_city_id;
                if (b->data.other.boatFigureId) {
                    trade_city_id = figure_get(b->data.other.boatFigureId)->empireCityId;
                } else {
                    trade_city_id = 0;
                }
                if (try_import_resource(f->destinationBuildingId, f->resourceId, trade_city_id)) {
                    int trader_id = figure_get(b->data.other.boatFigureId)->traderId;
                    trader_record_sold_resource(trader_id, f->resourceId);
                    f->actionState = FigureActionState_138_DockerImportReturning;
                    f->waitTicks = 0;
                    f->destinationX = f->sourceX;
                    f->destinationY = f->sourceY;
                    f->resourceId = 0;
                    fetch_export_resource(f, b);
                } else {
                    f->actionState = FigureActionState_138_DockerImportReturning;
                    f->destinationX = f->sourceX;
                    f->destinationY = f->sourceY;
                }
                f->waitTicks = 0;
            }
            f->graphicOffset = 0;
            break;
        case FigureActionState_140_DockerExportAtWarehouse:
            f->cartGraphicId = image_group(GROUP_FIGURE_CARTPUSHER_CART); // empty
            f->waitTicks++;
            if (f->waitTicks > 10) {
                int trade_city_id;
                if (b->data.other.boatFigureId) {
                    trade_city_id = figure_get(b->data.other.boatFigureId)->empireCityId;
                } else {
                    trade_city_id = 0;
                }
                f->actionState = FigureActionState_138_DockerImportReturning;
                f->destinationX = f->sourceX;
                f->destinationY = f->sourceY;
                f->waitTicks = 0;
                if (try_export_resource(f->destinationBuildingId, f->resourceId, trade_city_id)) {
                    int trader_id = figure_get(b->data.other.boatFigureId)->traderId;
                    trader_record_bought_resource(trader_id, f->resourceId);
                    f->actionState = FigureActionState_137_DockerExportReturning;
                } else {
                    fetch_export_resource(f, b);
                }
            }
            f->graphicOffset = 0;
            break;
    }

    int dir = figure_image_normalize_direction(f->direction < 8 ? f->direction : f->previousTileDirection);

    if (f->actionState == FigureActionState_149_Corpse) {
        f->graphicId = image_group(GROUP_FIGURE_CARTPUSHER) + figure_image_corpse_offset(f) + 96;
        f->cartGraphicId = 0;
    } else {
        f->graphicId = image_group(GROUP_FIGURE_CARTPUSHER) + dir + 8 * f->graphicOffset;
    }
    if (f->cartGraphicId) {
        f->cartGraphicId += dir;
        figure_image_set_cart_offset(f, dir);
    } else {
        f->graphicId = 0;
    }
}