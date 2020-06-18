//
// Created by liuliwu on 2020-06-08.
//

#ifndef TLBS_MFW_BUSI_H
#define TLBS_MFW_BUSI_H

#include <vector>
#include <string>
#include <map>

#include "exec.h"

namespace tLBS {

    class Connection;

    class MfwBusiness {
    private:
        static std::map<std::string, std::string> regionCountryMap; // 行政区划placeId所属国家区划placeId的map

    public:
        // region_fence [行政区划]
        // mfwS2RfSet regionPlaceId regionCountryPlaceId data
        static int execS2RfSet(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2RfGet regionPlaceId
        static int execS2RfGet(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2RfDel regionPlaceId
        static int execS2RfDel(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2RfLocate lat lon
        static int execS2RfLocate(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2RfNearby lat lon distance
        static int execS2RfNearby(Exec *exec, Connection *conn, std::vector<std::string> args);

        // poi_fence [poi圈]
        // mfwS2PfSet poiPlaceId data
        static int execS2PfSet(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2PfGet poiPlaceId
        static int execS2PfGet(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2PfDel poiPlaceId
        static int execS2PfDel(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2PfLocate lat lon
        static int execS2PfLocate(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2PfNearby lat lon distance
        static int execS2PfNearby(Exec *exec, Connection *conn, std::vector<std::string> args);

        // poi_point [poi点]
        // mfwS2PpSet poiPlaceId data
        static int execS2PpSet(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2PpGet poiPlaceId
        static int execS2PpGet(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2PpDel poiPlaceId
        static int execS2PpDel(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2PpNearby lat lon distance
        static int execS2PpNearby(Exec *exec, Connection *conn, std::vector<std::string> args);

        // area_fence [商圈]
        // mfwS2AfSet areaPlaceId data
        static int execS2AfSet(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2AfGet areaPlaceId
        static int execS2AfGet(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2AfDel areaPlaceId
        static int execS2AfDel(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2AfLocate lat lon
        static int execS2AfLocate(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2AfNearby lat lon distance
        static int execS2AfNearby(Exec *exec, Connection *conn, std::vector<std::string> args);

        // diy_fence [自己画的多边形]
        // mfwS2DfSet placeId data
        static int execS2DfSet(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2DfGet placeId
        static int execS2DfGet(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2DfDel placeid
        static int execS2DfDel(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2DfLocate lat lon
        static int execS2DfLocate(Exec *exec, Connection *conn, std::vector<std::string> args);
        // mfwS2DfNearby lat lon distance
        static int execS2DfNearby(Exec *exec, Connection *conn, std::vector<std::string> args);


        static void init();
        static void free();
    };
}


#endif //TLBS_MFW_BUSI_H
