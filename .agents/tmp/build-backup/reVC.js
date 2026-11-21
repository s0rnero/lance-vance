// include: shell.js
// include: minimum_runtime_check.js
// end include: minimum_runtime_check.js
// The Module object: Our interface to the outside world. We import
// and export values on it. There are various ways Module can be used:
// 1. Not defined. We create it here
// 2. A function parameter, function(moduleArg) => Promise<Module>
// 3. pre-run appended it, var Module = {}; ..generated code..
// 4. External script tag defines var Module.
// We need to check if Module already exists (e.g. case 3 above).
// Substitution will be replaced with actual code on later stage of the build,
// this way Closure Compiler will not mangle it (e.g. case 4. above).
// Note that if you want to run closure, and also to use Module
// after the generated code, you will need to define   var Module = {};
// before the code. Then that object will be used in the code, and you
// can continue to use Module afterwards as well.
var Module = typeof Module != 'undefined' ? Module : {};

// Determine the runtime environment we are in. You can customize this by
// setting the ENVIRONMENT setting at compile time (see settings.js).

// Attempt to auto-detect the environment
var ENVIRONMENT_IS_WEB = !!globalThis.window;
var ENVIRONMENT_IS_WORKER = !!globalThis.WorkerGlobalScope;
// N.b. Electron.js environment is simultaneously a NODE-environment, but
// also a web environment.
var ENVIRONMENT_IS_NODE = globalThis.process?.versions?.node && globalThis.process?.type != 'renderer';
var ENVIRONMENT_IS_SHELL = !ENVIRONMENT_IS_WEB && !ENVIRONMENT_IS_NODE && !ENVIRONMENT_IS_WORKER;

// --pre-jses are emitted after the Module integration code, so that they can
// refer to Module (if they choose; they can also define Module)
// include: C:\Users\s0rno\AppData\Local\Temp\tmp_hbod3e9.js

  if (!Module['expectedDataFileDownloads']) Module['expectedDataFileDownloads'] = 0;
  Module['expectedDataFileDownloads']++;
  (() => {
    // Do not attempt to redownload the virtual filesystem data when in a pthread or a Wasm Worker context.
    var isPthread = typeof ENVIRONMENT_IS_PTHREAD != 'undefined' && ENVIRONMENT_IS_PTHREAD;
    var isWasmWorker = typeof ENVIRONMENT_IS_WASM_WORKER != 'undefined' && ENVIRONMENT_IS_WASM_WORKER;
    if (isPthread || isWasmWorker) return;
    var isNode = globalThis.process && globalThis.process.versions && globalThis.process.versions.node && globalThis.process.type != 'renderer';
    async function loadPackage(metadata) {

      var PACKAGE_PATH = '';
      if (typeof window === 'object') {
        PACKAGE_PATH = window['encodeURIComponent'](window.location.pathname.substring(0, window.location.pathname.lastIndexOf('/')) + '/');
      } else if (typeof process === 'undefined' && typeof location !== 'undefined') {
        // web worker
        PACKAGE_PATH = encodeURIComponent(location.pathname.substring(0, location.pathname.lastIndexOf('/')) + '/');
      }
      var PACKAGE_NAME = 'C:/Users/s0rno/OneDrive/Documents/lance-vance/gta_vc_browser/web/public/build/reVC.data';
      var REMOTE_PACKAGE_BASE = 'reVC.data';
      var REMOTE_PACKAGE_NAME = Module['locateFile'] ? Module['locateFile'](REMOTE_PACKAGE_BASE, '') : REMOTE_PACKAGE_BASE;
      var REMOTE_PACKAGE_SIZE = metadata['remote_package_size'];

      async function fetchRemotePackage(packageName, packageSize) {
        if (isNode) {
          var contents = require('fs').readFileSync(packageName);
          return new Uint8Array(contents).buffer;
        }
        if (!Module['dataFileDownloads']) Module['dataFileDownloads'] = {};
        try {
          var response = await fetch(packageName);
        } catch (e) {
          throw new Error(`Network Error: ${packageName}`, {e});
        }
        if (!response.ok) {
          throw new Error(`${response.status}: ${response.url}`);
        }

        const chunks = [];
        const headers = response.headers;
        const total = Number(headers.get('Content-Length') || packageSize);
        let loaded = 0;

        Module['setStatus'] && Module['setStatus']('Downloading data...');
        const reader = response.body.getReader();

        while (1) {
          var {done, value} = await reader.read();
          if (done) break;
          chunks.push(value);
          loaded += value.length;
          Module['dataFileDownloads'][packageName] = {loaded, total};

          let totalLoaded = 0;
          let totalSize = 0;

          for (const download of Object.values(Module['dataFileDownloads'])) {
            totalLoaded += download.loaded;
            totalSize += download.total;
          }

          Module['setStatus'] && Module['setStatus'](`Downloading data... (${totalLoaded}/${totalSize})`);
        }

        const packageData = new Uint8Array(chunks.map((c) => c.length).reduce((a, b) => a + b, 0));
        let offset = 0;
        for (const chunk of chunks) {
          packageData.set(chunk, offset);
          offset += chunk.length;
        }
        return packageData.buffer;
      }

      var fetchPromise;
      var fetched = Module['getPreloadedPackage'] && Module['getPreloadedPackage'](REMOTE_PACKAGE_NAME, REMOTE_PACKAGE_SIZE);

      if (!fetched) {
        // Note that we don't use await here because we want to execute the
        // the rest of this function immediately.
        fetchPromise = fetchRemotePackage(REMOTE_PACKAGE_NAME, REMOTE_PACKAGE_SIZE);
      }

    async function runWithFS(Module) {

      function assert(check, msg) {
        if (!check) throw new Error(msg);
      }
Module['FS_createPath']("/", "Audio", true, true);
Module['FS_createPath']("/", "TEXT", true, true);
Module['FS_createPath']("/", "ViceExtended", true, true);
Module['FS_createPath']("/", "anim", true, true);
Module['FS_createPath']("/", "data", true, true);
Module['FS_createPath']("/data", "maps", true, true);
Module['FS_createPath']("/data/maps", "airport", true, true);
Module['FS_createPath']("/data/maps", "airportN", true, true);
Module['FS_createPath']("/data/maps", "bank", true, true);
Module['FS_createPath']("/data/maps", "bar", true, true);
Module['FS_createPath']("/data/maps", "bridge", true, true);
Module['FS_createPath']("/data/maps", "bryx", true, true);
Module['FS_createPath']("/data/maps", "cisland", true, true);
Module['FS_createPath']("/data/maps", "club", true, true);
Module['FS_createPath']("/data/maps", "concerth", true, true);
Module['FS_createPath']("/data/maps", "docks", true, true);
Module['FS_createPath']("/data/maps", "downtown", true, true);
Module['FS_createPath']("/data/maps", "downtows", true, true);
Module['FS_createPath']("/data/maps", "golf", true, true);
Module['FS_createPath']("/data/maps", "haiti", true, true);
Module['FS_createPath']("/data/maps", "haitiN", true, true);
Module['FS_createPath']("/data/maps", "hotel", true, true);
Module['FS_createPath']("/data/maps", "islandsf", true, true);
Module['FS_createPath']("/data/maps", "lawyers", true, true);
Module['FS_createPath']("/data/maps", "littleha", true, true);
Module['FS_createPath']("/data/maps", "mall", true, true);
Module['FS_createPath']("/data/maps", "mansion", true, true);
Module['FS_createPath']("/data/maps", "nbeach", true, true);
Module['FS_createPath']("/data/maps", "nbeachbt", true, true);
Module['FS_createPath']("/data/maps", "nbeachw", true, true);
Module['FS_createPath']("/data/maps", "newGen", true, true);
Module['FS_createPath']("/data/maps", "oceandn", true, true);
Module['FS_createPath']("/data/maps", "oceandrv", true, true);
Module['FS_createPath']("/data/maps", "plusroad", true, true);
Module['FS_createPath']("/data/maps", "stadint", true, true);
Module['FS_createPath']("/data/maps", "starisl", true, true);
Module['FS_createPath']("/data/maps", "stripclb", true, true);
Module['FS_createPath']("/data/maps", "washintn", true, true);
Module['FS_createPath']("/data/maps", "washints", true, true);
Module['FS_createPath']("/data/maps", "yacht", true, true);
Module['FS_createPath']("/data", "paths", true, true);
Module['FS_createPath']("/", "models", true, true);
Module['FS_createPath']("/models", "coll", true, true);
Module['FS_createPath']("/models", "generic", true, true);
Module['FS_createPath']("/models", "gta3.img", true, true);
Module['FS_createPath']("/", "neo", true, true);
Module['FS_createPath']("/", "skins", true, true);
Module['FS_createPath']("/", "txd", true, true);

      async function processPackageData(arrayBuffer) {
        assert(arrayBuffer, 'Loading data file failed.');
        assert(arrayBuffer.constructor.name === ArrayBuffer.name, 'bad input to processPackageData ' + arrayBuffer.constructor.name);
        var byteArray = new Uint8Array(arrayBuffer);
        var curr;
        // Reuse the bytearray from the XHR as the source for file reads.
          for (var file of metadata['files']) {
            var name = file['filename'];
            var data = byteArray.subarray(file['start'], file['end']);
            // canOwn this data in the filesystem, it is a slice into the heap that will never change
        Module['FS_createDataFile'](name, null, data, true, true, true);
          }
          Module['removeRunDependency']('datafile_C:/Users/s0rno/OneDrive/Documents/lance-vance/gta_vc_browser/web/public/build/reVC.data');
      }
      Module['addRunDependency']('datafile_C:/Users/s0rno/OneDrive/Documents/lance-vance/gta_vc_browser/web/public/build/reVC.data');

      if (!Module['preloadResults']) Module['preloadResults'] = {};

      Module['preloadResults'][PACKAGE_NAME] = {fromCache: false};
      if (!fetched) {
        fetched = await fetchPromise;
      }
      await processPackageData(fetched);

    }
    // Detect whether the module JS file has already been loaded.
    if (Module['FS_createPath']) {
      runWithFS(Module);
    } else {
      if (!Module['preRun']) Module['preRun'] = [];
      Module['preRun'].push(runWithFS); // FS is not initialized yet, wait for it
    }

    }
    loadPackage({"files": [{"filename": "/Audio/FLASH.adf", "start": 0, "end": 28703390}, {"filename": "/Audio/sound.cache", "start": 28703390, "end": 28708286}, {"filename": "/TEXT/american.gxt", "start": 28708286, "end": 29138168}, {"filename": "/TEXT/french.gxt", "start": 29138168, "end": 29615754}, {"filename": "/TEXT/german.gxt", "start": 29615754, "end": 30084476}, {"filename": "/TEXT/italian.gxt", "start": 30084476, "end": 30550306}, {"filename": "/TEXT/russian.gxt", "start": 30550306, "end": 30978356}, {"filename": "/TEXT/spanish.gxt", "start": 30978356, "end": 31441370}, {"filename": "/ViceExtended/features.ini", "start": 31441370, "end": 31441937}, {"filename": "/ViceExtended/limits.ini", "start": 31441937, "end": 31442174}, {"filename": "/anim/cuts.dir", "start": 31442174, "end": 31446910}, {"filename": "/anim/ped.ifp", "start": 31446910, "end": 33933266}, {"filename": "/data/Cullzone.dat", "start": 33933266, "end": 34088574}, {"filename": "/data/Cullzoneempty.dat", "start": 34088574, "end": 34249698}, {"filename": "/data/WATERPRO.DAT", "start": 34249698, "end": 34271142}, {"filename": "/data/animviewer.dat", "start": 34271142, "end": 34272275}, {"filename": "/data/carcols.dat", "start": 34272275, "end": 34280858}, {"filename": "/data/debug.sc", "start": 34280858, "end": 34286453}, {"filename": "/data/default.dat", "start": 34286453, "end": 34287059}, {"filename": "/data/default.ide", "start": 34287059, "end": 34309427}, {"filename": "/data/fistfite.dat", "start": 34309427, "end": 34311425}, {"filename": "/data/freeroam_miami.scm", "start": 34311425, "end": 34349622}, {"filename": "/data/gta_vc.dat", "start": 34349622, "end": 34352555}, {"filename": "/data/handling.cfg", "start": 34352555, "end": 34380230}, {"filename": "/data/info.zon", "start": 34380230, "end": 34390880}, {"filename": "/data/main.sc", "start": 34390880, "end": 34396636}, {"filename": "/data/main.scm", "start": 34396636, "end": 35587816}, {"filename": "/data/map.zon", "start": 35587816, "end": 35590005}, {"filename": "/data/maps/airport/airport.col", "start": 35590005, "end": 35914217}, {"filename": "/data/maps/airport/airport.ide", "start": 35914217, "end": 35930751}, {"filename": "/data/maps/airport/airport.ipl", "start": 35930751, "end": 35968888}, {"filename": "/data/maps/airportN/airportN.col", "start": 35968888, "end": 36179868}, {"filename": "/data/maps/airportN/airportN.ide", "start": 36179868, "end": 36184635}, {"filename": "/data/maps/airportN/airportN.ipl", "start": 36184635, "end": 36205837}, {"filename": "/data/maps/bank/bank.col", "start": 36205837, "end": 36223053}, {"filename": "/data/maps/bank/bank.ide", "start": 36223053, "end": 36230845}, {"filename": "/data/maps/bank/bank.ipl", "start": 36230845, "end": 36235886}, {"filename": "/data/maps/bar/bar.ide", "start": 36235886, "end": 36235930}, {"filename": "/data/maps/bridge/bridge.col", "start": 36235930, "end": 36335486}, {"filename": "/data/maps/bridge/bridge.ide", "start": 36335486, "end": 36337558}, {"filename": "/data/maps/bridge/bridge.ipl", "start": 36337558, "end": 36342449}, {"filename": "/data/maps/bryx/bryx.ide", "start": 36342449, "end": 36342560}, {"filename": "/data/maps/bryx/bryx.ipl", "start": 36342560, "end": 36342732}, {"filename": "/data/maps/cisland/cisland.col", "start": 36342732, "end": 36564024}, {"filename": "/data/maps/cisland/cisland.ide", "start": 36564024, "end": 36573213}, {"filename": "/data/maps/cisland/cisland.ipl", "start": 36573213, "end": 36599117}, {"filename": "/data/maps/club/CLUB.col", "start": 36599117, "end": 36623129}, {"filename": "/data/maps/club/CLUB.ipl", "start": 36623129, "end": 36629977}, {"filename": "/data/maps/club/club.ide", "start": 36629977, "end": 36639417}, {"filename": "/data/maps/concerth/concerth.col", "start": 36639417, "end": 36649997}, {"filename": "/data/maps/concerth/concerth.ide", "start": 36649997, "end": 36658264}, {"filename": "/data/maps/concerth/concerth.ipl", "start": 36658264, "end": 36660105}, {"filename": "/data/maps/cull.ipl", "start": 36660105, "end": 36717222}, {"filename": "/data/maps/docks/docks.col", "start": 36717222, "end": 37013666}, {"filename": "/data/maps/docks/docks.ide", "start": 37013666, "end": 37023027}, {"filename": "/data/maps/docks/docks.ipl", "start": 37023027, "end": 37053675}, {"filename": "/data/maps/downtown/downtown.col", "start": 37053675, "end": 37558463}, {"filename": "/data/maps/downtown/downtown.ide", "start": 37558463, "end": 37581512}, {"filename": "/data/maps/downtown/downtown.ipl", "start": 37581512, "end": 37636850}, {"filename": "/data/maps/downtows/downtows.col", "start": 37636850, "end": 37893114}, {"filename": "/data/maps/downtows/downtows.ide", "start": 37893114, "end": 37900100}, {"filename": "/data/maps/downtows/downtows.ipl", "start": 37900100, "end": 37926791}, {"filename": "/data/maps/generic.ide", "start": 37926791, "end": 37948918}, {"filename": "/data/maps/golf/golf.col", "start": 37948918, "end": 38309738}, {"filename": "/data/maps/golf/golf.ide", "start": 38309738, "end": 38315453}, {"filename": "/data/maps/golf/golf.ipl", "start": 38315453, "end": 38333017}, {"filename": "/data/maps/haiti/haiti.col", "start": 38333017, "end": 38685549}, {"filename": "/data/maps/haiti/haiti.ide", "start": 38685549, "end": 38691513}, {"filename": "/data/maps/haiti/haiti.ipl", "start": 38691513, "end": 38730816}, {"filename": "/data/maps/haitiN/haitiN.ide", "start": 38730816, "end": 38736761}, {"filename": "/data/maps/haitiN/haitin.col", "start": 38736761, "end": 39038105}, {"filename": "/data/maps/haitiN/haitin.ipl", "start": 39038105, "end": 39064794}, {"filename": "/data/maps/hotel/hotel.col", "start": 39064794, "end": 39075974}, {"filename": "/data/maps/hotel/hotel.ide", "start": 39075974, "end": 39080735}, {"filename": "/data/maps/hotel/hotel.ipl", "start": 39080735, "end": 39089770}, {"filename": "/data/maps/islandsf/islandsf.col", "start": 39089770, "end": 39098326}, {"filename": "/data/maps/islandsf/islandsf.ide", "start": 39098326, "end": 39102330}, {"filename": "/data/maps/islandsf/islandsf.ipl", "start": 39102330, "end": 39112710}, {"filename": "/data/maps/lawyers/lawyers.col", "start": 39112710, "end": 39128502}, {"filename": "/data/maps/lawyers/lawyers.ide", "start": 39128502, "end": 39129600}, {"filename": "/data/maps/lawyers/lawyers.ipl", "start": 39129600, "end": 39131038}, {"filename": "/data/maps/littleha/littleha.col", "start": 39131038, "end": 39498598}, {"filename": "/data/maps/littleha/littleha.ide", "start": 39498598, "end": 39514171}, {"filename": "/data/maps/littleha/littleha.ipl", "start": 39514171, "end": 39583078}, {"filename": "/data/maps/mall/mall.col", "start": 39583078, "end": 39785570}, {"filename": "/data/maps/mall/mall.ide", "start": 39785570, "end": 39795594}, {"filename": "/data/maps/mall/mall.ipl", "start": 39795594, "end": 39819642}, {"filename": "/data/maps/mansion/mansion.col", "start": 39819642, "end": 39972518}, {"filename": "/data/maps/mansion/mansion.ide", "start": 39972518, "end": 39982202}, {"filename": "/data/maps/mansion/mansion.ipl", "start": 39982202, "end": 39997517}, {"filename": "/data/maps/map0.dat", "start": 39997517, "end": 39998208}, {"filename": "/data/maps/map1.dat", "start": 39998208, "end": 39998899}, {"filename": "/data/maps/map2.dat", "start": 39998899, "end": 39999590}, {"filename": "/data/maps/map3.dat", "start": 39999590, "end": 40000281}, {"filename": "/data/maps/map4.dat", "start": 40000281, "end": 40000972}, {"filename": "/data/maps/map5.dat", "start": 40000972, "end": 40001663}, {"filename": "/data/maps/map6.dat", "start": 40001663, "end": 40002354}, {"filename": "/data/maps/map7.dat", "start": 40002354, "end": 40003045}, {"filename": "/data/maps/nbeach/nbeach.col", "start": 40003045, "end": 40359745}, {"filename": "/data/maps/nbeach/nbeach.ide", "start": 40359745, "end": 40365404}, {"filename": "/data/maps/nbeach/nbeach.ipl", "start": 40365404, "end": 40408693}, {"filename": "/data/maps/nbeachbt/nbeachbt.col", "start": 40408693, "end": 40904081}, {"filename": "/data/maps/nbeachbt/nbeachbt.ide", "start": 40904081, "end": 40914445}, {"filename": "/data/maps/nbeachbt/nbeachbt.ipl", "start": 40914445, "end": 40989445}, {"filename": "/data/maps/nbeachw/nbeachw.col", "start": 40989445, "end": 41251025}, {"filename": "/data/maps/nbeachw/nbeachw.ide", "start": 41251025, "end": 41260533}, {"filename": "/data/maps/nbeachw/nbeachw.ipl", "start": 41260533, "end": 41301898}, {"filename": "/data/maps/newGen/newgen.IDE", "start": 41301898, "end": 41301993}, {"filename": "/data/maps/newGen/newgen.IPL", "start": 41301993, "end": 41302099}, {"filename": "/data/maps/newGen/newgen.col", "start": 41302099, "end": 41316247}, {"filename": "/data/maps/oceandn/oceandN.col", "start": 41316247, "end": 41606939}, {"filename": "/data/maps/oceandn/oceandN.ide", "start": 41606939, "end": 41627591}, {"filename": "/data/maps/oceandn/oceandN.ipl", "start": 41627591, "end": 41705328}, {"filename": "/data/maps/oceandrv/oceandrv.col", "start": 41705328, "end": 41882436}, {"filename": "/data/maps/oceandrv/oceandrv.ide", "start": 41882436, "end": 41895865}, {"filename": "/data/maps/oceandrv/oceandrv.ipl", "start": 41895865, "end": 41946531}, {"filename": "/data/maps/paths.ipl", "start": 41946531, "end": 43118087}, {"filename": "/data/maps/plusroad/plusroad.IDE", "start": 43118087, "end": 43118392}, {"filename": "/data/maps/plusroad/plusroad.ipl", "start": 43118392, "end": 43120163}, {"filename": "/data/maps/stadint/stadint.col", "start": 43120163, "end": 43553499}, {"filename": "/data/maps/stadint/stadint.ide", "start": 43553499, "end": 43563977}, {"filename": "/data/maps/stadint/stadint.ipl", "start": 43563977, "end": 43571334}, {"filename": "/data/maps/starisl/starisl.col", "start": 43571334, "end": 43817854}, {"filename": "/data/maps/starisl/starisl.ide", "start": 43817854, "end": 43821958}, {"filename": "/data/maps/starisl/starisl.ipl", "start": 43821958, "end": 43861368}, {"filename": "/data/maps/stripclb/stripclb.col", "start": 43861368, "end": 43890360}, {"filename": "/data/maps/stripclb/stripclb.ide", "start": 43890360, "end": 43895567}, {"filename": "/data/maps/stripclb/stripclb.ipl", "start": 43895567, "end": 43896755}, {"filename": "/data/maps/wanted_paths.ipl", "start": 43896755, "end": 43919183}, {"filename": "/data/maps/washintn/washintn.col", "start": 43919183, "end": 44194295}, {"filename": "/data/maps/washintn/washintn.ide", "start": 44194295, "end": 44212798}, {"filename": "/data/maps/washintn/washintn.ipl", "start": 44212798, "end": 44296592}, {"filename": "/data/maps/washints/washints.col", "start": 44296592, "end": 44605796}, {"filename": "/data/maps/washints/washints.ide", "start": 44605796, "end": 44627465}, {"filename": "/data/maps/washints/washints.ipl", "start": 44627465, "end": 44695671}, {"filename": "/data/maps/yacht/yacht.col", "start": 44695671, "end": 44720607}, {"filename": "/data/maps/yacht/yacht.ide", "start": 44720607, "end": 44723020}, {"filename": "/data/maps/yacht/yacht.ipl", "start": 44723020, "end": 44723105}, {"filename": "/data/navig.zon", "start": 44723105, "end": 44723878}, {"filename": "/data/object.dat", "start": 44723878, "end": 44750657}, {"filename": "/data/occlu.ipl", "start": 44750657, "end": 44772602}, {"filename": "/data/particle.cfg", "start": 44772602, "end": 44790965}, {"filename": "/data/paths/flight.dat", "start": 44790965, "end": 44799341}, {"filename": "/data/paths/flight2.dat", "start": 44799341, "end": 44802955}, {"filename": "/data/paths/flight3.dat", "start": 44802955, "end": 44815245}, {"filename": "/data/paths/spath0.dat", "start": 44815245, "end": 44817131}, {"filename": "/data/ped.dat", "start": 44817131, "end": 44819197}, {"filename": "/data/pedgrp.dat", "start": 44819197, "end": 44827705}, {"filename": "/data/pedstats.dat", "start": 44827705, "end": 44830984}, {"filename": "/data/surface.dat", "start": 44830984, "end": 44831560}, {"filename": "/data/timecyc.dat", "start": 44831560, "end": 44878089}, {"filename": "/data/train.dat", "start": 44878089, "end": 44880565}, {"filename": "/data/train2.dat", "start": 44880565, "end": 44884797}, {"filename": "/data/water.dat", "start": 44884797, "end": 44886116}, {"filename": "/data/weapon.dat", "start": 44886116, "end": 44894377}, {"filename": "/gamecontrollerdb.txt", "start": 44894377, "end": 45147471}, {"filename": "/models/INTRO.TXD", "start": 45147471, "end": 45232207}, {"filename": "/models/MISC.TXD", "start": 45232207, "end": 45256183}, {"filename": "/models/coll/generic.col", "start": 45256183, "end": 45340955}, {"filename": "/models/coll/newWeapons.col", "start": 45340955, "end": 45341179}, {"filename": "/models/coll/newvehicles.col", "start": 45341179, "end": 45346583}, {"filename": "/models/coll/peds.col", "start": 45346583, "end": 45368060}, {"filename": "/models/coll/vehicles.col", "start": 45368060, "end": 45473656}, {"filename": "/models/coll/weapons.col", "start": 45473656, "end": 45474104}, {"filename": "/models/fonts.txd", "start": 45474104, "end": 45998688}, {"filename": "/models/fonts_r.txd", "start": 45998688, "end": 46523272}, {"filename": "/models/fronten1.txd", "start": 46523272, "end": 46721328}, {"filename": "/models/fronten2.txd", "start": 46721328, "end": 59110744}, {"filename": "/models/frontend_ds2.txd", "start": 59110744, "end": 59440720}, {"filename": "/models/frontend_ds3.txd", "start": 59440720, "end": 59965688}, {"filename": "/models/frontend_ds4.txd", "start": 59965688, "end": 60490656}, {"filename": "/models/frontend_ds5.txd", "start": 60490656, "end": 60818632}, {"filename": "/models/frontend_nsw.txd", "start": 60818632, "end": 63178736}, {"filename": "/models/frontend_x360.txd", "start": 63178736, "end": 63703704}, {"filename": "/models/frontend_xone.txd", "start": 63703704, "end": 64228672}, {"filename": "/models/generic.txd", "start": 64228672, "end": 65778036}, {"filename": "/models/generic/air_vlo.DFF", "start": 65778036, "end": 65784952}, {"filename": "/models/generic/arrow.DFF", "start": 65784952, "end": 65789801}, {"filename": "/models/generic/player.bmp", "start": 65789801, "end": 65856413}, {"filename": "/models/generic/wheels.DFF", "start": 65856413, "end": 67737893}, {"filename": "/models/generic/wheels.TXD", "start": 67737893, "end": 69376521}, {"filename": "/models/generic/zonecylb.DFF", "start": 69376521, "end": 69378781}, {"filename": "/models/gta3.dir", "start": 69378781, "end": 69573853}, {"filename": "/models/gta3.img/admiral.dff", "start": 69573853, "end": 70524670}, {"filename": "/models/gta3.img/admiral.txd", "start": 70524670, "end": 72192534}, {"filename": "/models/gta3.img/air_brway_030.dff", "start": 72192534, "end": 72219158}, {"filename": "/models/gta3.img/air_brway_34.dff", "start": 72219158, "end": 72245782}, {"filename": "/models/gta3.img/airgrndb.txd", "start": 72245782, "end": 72256022}, {"filename": "/models/gta3.img/airplan.dff", "start": 72256022, "end": 72325654}, {"filename": "/models/gta3.img/airplan.txd", "start": 72325654, "end": 72464918}, {"filename": "/models/gta3.img/airport.col", "start": 72464918, "end": 72792598}, {"filename": "/models/gta3.img/airport.txd", "start": 72792598, "end": 72942102}, {"filename": "/models/gta3.img/airporterminal.txd", "start": 72942102, "end": 73206294}, {"filename": "/models/gta3.img/airportn.col", "start": 73206294, "end": 73419286}, {"filename": "/models/gta3.img/airsignn.txd", "start": 73419286, "end": 73507350}, {"filename": "/models/gta3.img/airstuff2.txd", "start": 73507350, "end": 73533974}, {"filename": "/models/gta3.img/airtrain.dff", "start": 73533974, "end": 73628182}, {"filename": "/models/gta3.img/airtrain.txd", "start": 73628182, "end": 73654806}, {"filename": "/models/gta3.img/ak47.dff", "start": 73654806, "end": 73675286}, {"filename": "/models/gta3.img/ak47.txd", "start": 73675286, "end": 73749014}, {"filename": "/models/gta3.img/alleyprop.txd", "start": 73749014, "end": 73775638}, {"filename": "/models/gta3.img/ap_3dbillb.dff", "start": 73775638, "end": 73841174}, {"filename": "/models/gta3.img/ap_barriergate3.dff", "start": 73841174, "end": 73863702}, {"filename": "/models/gta3.img/ap_billboard.txd", "start": 73863702, "end": 74099222}, {"filename": "/models/gta3.img/ap_billboards3.dff", "start": 74099222, "end": 74121750}, {"filename": "/models/gta3.img/ap_blastdef_03.dff", "start": 74121750, "end": 74134038}, {"filename": "/models/gta3.img/ap_boardshad1.dff", "start": 74134038, "end": 74140182}, {"filename": "/models/gta3.img/ap_build1.txd", "start": 74140182, "end": 74220054}, {"filename": "/models/gta3.img/ap_build2.txd", "start": 74220054, "end": 74578454}, {"filename": "/models/gta3.img/ap_buildings2.txd", "start": 74578454, "end": 74850838}, {"filename": "/models/gta3.img/ap_carbush2_01.dff", "start": 74850838, "end": 74881558}, {"filename": "/models/gta3.img/ap_carparkterm1.dff", "start": 74881558, "end": 74957334}, {"filename": "/models/gta3.img/ap_clothes.txd", "start": 74957334, "end": 75043350}, {"filename": "/models/gta3.img/ap_dockstairs1.dff", "start": 75043350, "end": 75076118}, {"filename": "/models/gta3.img/ap_firetrucks.dff", "start": 75076118, "end": 75104790}, {"filename": "/models/gta3.img/ap_francisland.txd", "start": 75104790, "end": 75295254}, {"filename": "/models/gta3.img/ap_ground1.txd", "start": 75295254, "end": 75444758}, {"filename": "/models/gta3.img/ap_hangar1_01.dff", "start": 75444758, "end": 75512342}, {"filename": "/models/gta3.img/ap_hland_01.dff", "start": 75512342, "end": 75524630}, {"filename": "/models/gta3.img/ap_jumbo_01.dff", "start": 75524630, "end": 75581974}, {"filename": "/models/gta3.img/ap_jumbo_bridge.dff", "start": 75581974, "end": 75639318}, {"filename": "/models/gta3.img/ap_jumbos.txd", "start": 75639318, "end": 75809302}, {"filename": "/models/gta3.img/ap_learjet1_01.dff", "start": 75809302, "end": 75860502}, {"filename": "/models/gta3.img/ap_learjets.txd", "start": 75860502, "end": 75872790}, {"filename": "/models/gta3.img/ap_lod.txd", "start": 75872790, "end": 75913750}, {"filename": "/models/gta3.img/ap_lodbit.txd", "start": 75913750, "end": 75917846}, {"filename": "/models/gta3.img/ap_misc1bit.txd", "start": 75917846, "end": 75985430}, {"filename": "/models/gta3.img/ap_newesca_01.dff", "start": 75985430, "end": 76065302}, {"filename": "/models/gta3.img/ap_newescaglas_01.dff", "start": 76065302, "end": 76083734}, {"filename": "/models/gta3.img/ap_newprops1opac.txd", "start": 76083734, "end": 76100118}, {"filename": "/models/gta3.img/ap_radar1_01.dff", "start": 76100118, "end": 76120598}, {"filename": "/models/gta3.img/ap_rland04.dff", "start": 76120598, "end": 76141078}, {"filename": "/models/gta3.img/ap_rland06.dff", "start": 76141078, "end": 76159510}, {"filename": "/models/gta3.img/ap_rland07.dff", "start": 76159510, "end": 76167702}, {"filename": "/models/gta3.img/ap_rland10.dff", "start": 76167702, "end": 76184086}, {"filename": "/models/gta3.img/ap_rlandnew9.dff", "start": 76184086, "end": 76204566}, {"filename": "/models/gta3.img/ap_roads.txd", "start": 76204566, "end": 76515862}, {"filename": "/models/gta3.img/ap_roadsect1.dff", "start": 76515862, "end": 76542486}, {"filename": "/models/gta3.img/ap_roadsect1.txd", "start": 76542486, "end": 76685846}, {"filename": "/models/gta3.img/ap_roadsect2a.dff", "start": 76685846, "end": 76712470}, {"filename": "/models/gta3.img/ap_roadsect3.dff", "start": 76712470, "end": 76751382}, {"filename": "/models/gta3.img/ap_roadsect3b.dff", "start": 76751382, "end": 76775958}, {"filename": "/models/gta3.img/ap_roadsect4.dff", "start": 76775958, "end": 76913174}, {"filename": "/models/gta3.img/ap_roadsect5.dff", "start": 76913174, "end": 76958230}, {"filename": "/models/gta3.img/ap_runsign3_01.dff", "start": 76958230, "end": 76970518}, {"filename": "/models/gta3.img/ap_runsigns1.txd", "start": 76970518, "end": 77072918}, {"filename": "/models/gta3.img/ap_seaplanedock1.dff", "start": 77072918, "end": 77093398}, {"filename": "/models/gta3.img/ap_seaplanprop.txd", "start": 77093398, "end": 77412886}, {"filename": "/models/gta3.img/ap_smahangar2_01.dff", "start": 77412886, "end": 77416982}, {"filename": "/models/gta3.img/ap_smallradar1_02.dff", "start": 77416982, "end": 77427222}, {"filename": "/models/gta3.img/ap_steps1_01.dff", "start": 77427222, "end": 77449750}, {"filename": "/models/gta3.img/ap_stepsn1_01.dff", "start": 77449750, "end": 77472278}, {"filename": "/models/gta3.img/ap_termbbase1.dff", "start": 77472278, "end": 77511190}, {"filename": "/models/gta3.img/ap_termina2_01.dff", "start": 77511190, "end": 77539862}, {"filename": "/models/gta3.img/ap_termina_01.dff", "start": 77539862, "end": 77568534}, {"filename": "/models/gta3.img/ap_termina_roof1.dff", "start": 77568534, "end": 77578774}, {"filename": "/models/gta3.img/ap_termina_roof2.dff", "start": 77578774, "end": 77589014}, {"filename": "/models/gta3.img/ap_terminagrflo1.dff", "start": 77589014, "end": 77597206}, {"filename": "/models/gta3.img/ap_terminagrflo2.dff", "start": 77597206, "end": 77605398}, {"filename": "/models/gta3.img/ap_terminal1.txd", "start": 77605398, "end": 77879830}, {"filename": "/models/gta3.img/ap_terminal1boards.txd", "start": 77879830, "end": 78160406}, {"filename": "/models/gta3.img/ap_terminalb1.dff", "start": 78160406, "end": 78244374}, {"filename": "/models/gta3.img/ap_termsign1_dy.dff", "start": 78244374, "end": 78246422}, {"filename": "/models/gta3.img/ap_termwindows.txd", "start": 78246422, "end": 78414358}, {"filename": "/models/gta3.img/ap_termwindows1.dff", "start": 78414358, "end": 78426646}, {"filename": "/models/gta3.img/ap_termwindows1b.dff", "start": 78426646, "end": 78432790}, {"filename": "/models/gta3.img/ap_tower.dff", "start": 78432790, "end": 78453270}, {"filename": "/models/gta3.img/ap_trailer_01.dff", "start": 78453270, "end": 78500374}, {"filename": "/models/gta3.img/ap_treesfw1_01.dff", "start": 78500374, "end": 78508566}, {"filename": "/models/gta3.img/ap_treeshot1_02.dff", "start": 78508566, "end": 78522902}, {"filename": "/models/gta3.img/ap_viceairport.txd", "start": 78522902, "end": 78713366}, {"filename": "/models/gta3.img/ap_wallfence2.dff", "start": 78713366, "end": 78725654}, {"filename": "/models/gta3.img/ap_wallfence3.dff", "start": 78725654, "end": 78739990}, {"filename": "/models/gta3.img/ap_wallfence4.dff", "start": 78739990, "end": 78756374}, {"filename": "/models/gta3.img/ap_wallfence5.dff", "start": 78756374, "end": 78774806}, {"filename": "/models/gta3.img/ap_wallfence7.dff", "start": 78774806, "end": 78782998}, {"filename": "/models/gta3.img/ap_windowstruts.dff", "start": 78782998, "end": 78842390}, {"filename": "/models/gta3.img/apairprtbits07.dff", "start": 78842390, "end": 78846486}, {"filename": "/models/gta3.img/apcarparkn.txd", "start": 78846486, "end": 78907926}, {"filename": "/models/gta3.img/apchecpointn.txd", "start": 78907926, "end": 78985750}, {"filename": "/models/gta3.img/apgroundn.txd", "start": 78985750, "end": 79077910}, {"filename": "/models/gta3.img/apjumbon.txd", "start": 79077910, "end": 79247894}, {"filename": "/models/gta3.img/aproadsn.txd", "start": 79247894, "end": 79528470}, {"filename": "/models/gta3.img/b_hse_doors.dff", "start": 79528470, "end": 79530518}, {"filename": "/models/gta3.img/b_hse_ext.dff", "start": 79530518, "end": 79555094}, {"filename": "/models/gta3.img/b_hse_interior.dff", "start": 79555094, "end": 79579670}, {"filename": "/models/gta3.img/b_hse_interiorrays.dff", "start": 79579670, "end": 79583766}, {"filename": "/models/gta3.img/b_hse_pier.dff", "start": 79583766, "end": 79708694}, {"filename": "/models/gta3.img/b_hse_pierfence.dff", "start": 79708694, "end": 79720982}, {"filename": "/models/gta3.img/baggage.dff", "start": 79720982, "end": 79971985}, {"filename": "/models/gta3.img/baggage.txd", "start": 79971985, "end": 80721441}, {"filename": "/models/gta3.img/bank.col", "start": 80721441, "end": 80739873}, {"filename": "/models/gta3.img/banshee.dff", "start": 80739873, "end": 81631286}, {"filename": "/models/gta3.img/banshee.txd", "start": 81631286, "end": 82927482}, {"filename": "/models/gta3.img/baseball.ifp", "start": 82927482, "end": 82986874}, {"filename": "/models/gta3.img/basketballcourt04.dff", "start": 82986874, "end": 83009402}, {"filename": "/models/gta3.img/bat.dff", "start": 83009402, "end": 83013498}, {"filename": "/models/gta3.img/bat.txd", "start": 83013498, "end": 83085178}, {"filename": "/models/gta3.img/beach_bush02.dff", "start": 83085178, "end": 83087226}, {"filename": "/models/gta3.img/beach_bush06s.dff", "start": 83087226, "end": 83091322}, {"filename": "/models/gta3.img/beach_bush08s.dff", "start": 83091322, "end": 83095418}, {"filename": "/models/gta3.img/beachlo1.dff", "start": 83095418, "end": 83099514}, {"filename": "/models/gta3.img/benson.dff", "start": 83099514, "end": 83468479}, {"filename": "/models/gta3.img/benson.txd", "start": 83468479, "end": 83949743}, {"filename": "/models/gta3.img/beretta.dff", "start": 83949743, "end": 83962031}, {"filename": "/models/gta3.img/beretta.txd", "start": 83962031, "end": 84035759}, {"filename": "/models/gta3.img/bfinject.dff", "start": 84035759, "end": 84331135}, {"filename": "/models/gta3.img/bfinject.txd", "start": 84331135, "end": 84370047}, {"filename": "/models/gta3.img/bfost.dff", "start": 84370047, "end": 84437631}, {"filename": "/models/gta3.img/bfost.txd", "start": 84437631, "end": 84525695}, {"filename": "/models/gta3.img/bfotr.dff", "start": 84525695, "end": 84591231}, {"filename": "/models/gta3.img/bfotr.txd", "start": 84591231, "end": 84636287}, {"filename": "/models/gta3.img/bfypr.dff", "start": 84636287, "end": 84695679}, {"filename": "/models/gta3.img/bfypr.txd", "start": 84695679, "end": 84740735}, {"filename": "/models/gta3.img/bfyst.dff", "start": 84740735, "end": 84806271}, {"filename": "/models/gta3.img/bfyst.txd", "start": 84806271, "end": 84959871}, {"filename": "/models/gta3.img/biked.ifp", "start": 84959871, "end": 85123711}, {"filename": "/models/gta3.img/bikeh.ifp", "start": 85123711, "end": 85254783}, {"filename": "/models/gta3.img/bikes.ifp", "start": 85254783, "end": 85410431}, {"filename": "/models/gta3.img/bikev.ifp", "start": 85410431, "end": 85512831}, {"filename": "/models/gta3.img/billbd3.dff", "start": 85512831, "end": 85541503}, {"filename": "/models/gta3.img/billbrd.txd", "start": 85541503, "end": 85592703}, {"filename": "/models/gta3.img/blimp_day.dff", "start": 85592703, "end": 85604991}, {"filename": "/models/gta3.img/blimp_night.dff", "start": 85604991, "end": 85617279}, {"filename": "/models/gta3.img/blistac.dff", "start": 85617279, "end": 86120180}, {"filename": "/models/gta3.img/blistac.txd", "start": 86120180, "end": 86972976}, {"filename": "/models/gta3.img/bmodk.dff", "start": 86972976, "end": 87044656}, {"filename": "/models/gta3.img/bmodk.txd", "start": 87044656, "end": 87132720}, {"filename": "/models/gta3.img/bmost.dff", "start": 87132720, "end": 87212592}, {"filename": "/models/gta3.img/bmost.txd", "start": 87212592, "end": 87300656}, {"filename": "/models/gta3.img/bmotr.dff", "start": 87300656, "end": 87376432}, {"filename": "/models/gta3.img/bmotr.txd", "start": 87376432, "end": 87444016}, {"filename": "/models/gta3.img/bmycr.dff", "start": 87444016, "end": 87517744}, {"filename": "/models/gta3.img/bmycr.txd", "start": 87517744, "end": 87605808}, {"filename": "/models/gta3.img/bmypi.dff", "start": 87605808, "end": 87685680}, {"filename": "/models/gta3.img/bmypi.txd", "start": 87685680, "end": 87773744}, {"filename": "/models/gta3.img/bmyst.dff", "start": 87773744, "end": 87849520}, {"filename": "/models/gta3.img/bmyst.txd", "start": 87849520, "end": 87937584}, {"filename": "/models/gta3.img/boat_kb1.dff", "start": 87937584, "end": 87956016}, {"filename": "/models/gta3.img/boat_kb2.dff", "start": 87956016, "end": 88029744}, {"filename": "/models/gta3.img/boatcranelg0.dff", "start": 88029744, "end": 88060464}, {"filename": "/models/gta3.img/boatcranesm0.dff", "start": 88060464, "end": 88087088}, {"filename": "/models/gta3.img/boathouse.txd", "start": 88087088, "end": 88574512}, {"filename": "/models/gta3.img/bobcat.dff", "start": 88574512, "end": 89003202}, {"filename": "/models/gta3.img/bobcat.txd", "start": 89003202, "end": 89772698}, {"filename": "/models/gta3.img/bodyarmour.dff", "start": 89772698, "end": 89780890}, {"filename": "/models/gta3.img/bollardlight.dff", "start": 89780890, "end": 89784986}, {"filename": "/models/gta3.img/boxville.dff", "start": 89784986, "end": 90091804}, {"filename": "/models/gta3.img/boxville.txd", "start": 90091804, "end": 90568716}, {"filename": "/models/gta3.img/brassknuckle.dff", "start": 90568716, "end": 90572812}, {"filename": "/models/gta3.img/brassknuckle.txd", "start": 90572812, "end": 90642444}, {"filename": "/models/gta3.img/brfcase.dff", "start": 90642444, "end": 90669068}, {"filename": "/models/gta3.img/brfcase.txd", "start": 90669068, "end": 90683404}, {"filename": "/models/gta3.img/bribe.dff", "start": 90683404, "end": 90689548}, {"filename": "/models/gta3.img/bridge.col", "start": 90689548, "end": 90789900}, {"filename": "/models/gta3.img/buddy.ifp", "start": 90789900, "end": 90832908}, {"filename": "/models/gta3.img/burrito.dff", "start": 90832908, "end": 91366392}, {"filename": "/models/gta3.img/burrito.txd", "start": 91366392, "end": 91757832}, {"filename": "/models/gta3.img/bus.dff", "start": 91757832, "end": 92194512}, {"filename": "/models/gta3.img/bus.txd", "start": 92194512, "end": 92960168}, {"filename": "/models/gta3.img/bussign1.dff", "start": 92960168, "end": 92964264}, {"filename": "/models/gta3.img/cabbie.dff", "start": 92964264, "end": 93762789}, {"filename": "/models/gta3.img/cabbie.txd", "start": 93762789, "end": 94900205}, {"filename": "/models/gta3.img/cdseaan.dff", "start": 94900205, "end": 94902253}, {"filename": "/models/gta3.img/cdseabed01.dff", "start": 94902253, "end": 94908397}, {"filename": "/models/gta3.img/cdseabed02.dff", "start": 94908397, "end": 94914541}, {"filename": "/models/gta3.img/cdseabed03.dff", "start": 94914541, "end": 94918637}, {"filename": "/models/gta3.img/cdseabed04.dff", "start": 94918637, "end": 94922733}, {"filename": "/models/gta3.img/cdseabed05.dff", "start": 94922733, "end": 94926829}, {"filename": "/models/gta3.img/cdseabed06.dff", "start": 94926829, "end": 94930925}, {"filename": "/models/gta3.img/cdseabed07.dff", "start": 94930925, "end": 94935021}, {"filename": "/models/gta3.img/cdseabed08.dff", "start": 94935021, "end": 94939117}, {"filename": "/models/gta3.img/cdseabed09.dff", "start": 94939117, "end": 94943213}, {"filename": "/models/gta3.img/cdseabed16.dff", "start": 94943213, "end": 94945261}, {"filename": "/models/gta3.img/cdseabed18.dff", "start": 94945261, "end": 94949357}, {"filename": "/models/gta3.img/cdseabed19.dff", "start": 94949357, "end": 94951405}, {"filename": "/models/gta3.img/cdseabed20.dff", "start": 94951405, "end": 94955501}, {"filename": "/models/gta3.img/cdseabed21.dff", "start": 94955501, "end": 94957549}, {"filename": "/models/gta3.img/cdseabed24.dff", "start": 94957549, "end": 94959597}, {"filename": "/models/gta3.img/cdseabed25.dff", "start": 94959597, "end": 94961645}, {"filename": "/models/gta3.img/cdseabed26.dff", "start": 94961645, "end": 94963693}, {"filename": "/models/gta3.img/cdseabed27.dff", "start": 94963693, "end": 94965741}, {"filename": "/models/gta3.img/cdseabed28.dff", "start": 94965741, "end": 94967789}, {"filename": "/models/gta3.img/cdseabed29.dff", "start": 94967789, "end": 94969837}, {"filename": "/models/gta3.img/cdseabed30.dff", "start": 94969837, "end": 94971885}, {"filename": "/models/gta3.img/cheetah.dff", "start": 94971885, "end": 95724113}, {"filename": "/models/gta3.img/cheetah.txd", "start": 95724113, "end": 97042293}, {"filename": "/models/gta3.img/chromegun.dff", "start": 97042293, "end": 97052533}, {"filename": "/models/gta3.img/chromegun.txd", "start": 97052533, "end": 97124213}, {"filename": "/models/gta3.img/cisland.col", "start": 97124213, "end": 97347445}, {"filename": "/models/gta3.img/cl_tablesetlrg.dff", "start": 97347445, "end": 97374069}, {"filename": "/models/gta3.img/club.col", "start": 97374069, "end": 97398645}, {"filename": "/models/gta3.img/coach.dff", "start": 97398645, "end": 97856294}, {"filename": "/models/gta3.img/coach.ifp", "start": 97856294, "end": 97942310}, {"filename": "/models/gta3.img/coach.txd", "start": 97942310, "end": 98340150}, {"filename": "/models/gta3.img/cokpoolwater.dff", "start": 98340150, "end": 98342198}, {"filename": "/models/gta3.img/colt45.dff", "start": 98342198, "end": 98354486}, {"filename": "/models/gta3.img/colt45.ifp", "start": 98354486, "end": 98413878}, {"filename": "/models/gta3.img/colt45.txd", "start": 98413878, "end": 98493750}, {"filename": "/models/gta3.img/comet.dff", "start": 98493750, "end": 99224087}, {"filename": "/models/gta3.img/comet.txd", "start": 99224087, "end": 99618343}, {"filename": "/models/gta3.img/concerth.col", "start": 99618343, "end": 99630631}, {"filename": "/models/gta3.img/cop.dff", "start": 99630631, "end": 99708455}, {"filename": "/models/gta3.img/cop.txd", "start": 99708455, "end": 99993127}, {"filename": "/models/gta3.img/cranebasea0.dff", "start": 99993127, "end": 100017703}, {"filename": "/models/gta3.img/cranes.txd", "start": 100017703, "end": 100093479}, {"filename": "/models/gta3.img/crate.txd", "start": 100093479, "end": 100435495}, {"filename": "/models/gta3.img/crgoshp010.dff", "start": 100435495, "end": 100513319}, {"filename": "/models/gta3.img/cs_chop.dff", "start": 100513319, "end": 100759079}, {"filename": "/models/gta3.img/cs_chop.txd", "start": 100759079, "end": 100877863}, {"filename": "/models/gta3.img/csassa.dff", "start": 100877863, "end": 100967975}, {"filename": "/models/gta3.img/csassa.txd", "start": 100967975, "end": 101013031}, {"filename": "/models/gta3.img/csassb.dff", "start": 101013031, "end": 101107239}, {"filename": "/models/gta3.img/csassb.txd", "start": 101107239, "end": 101152295}, {"filename": "/models/gta3.img/csassc.dff", "start": 101152295, "end": 101242407}, {"filename": "/models/gta3.img/csassc.txd", "start": 101242407, "end": 101287463}, {"filename": "/models/gta3.img/csdeal.dff", "start": 101287463, "end": 101420583}, {"filename": "/models/gta3.img/csdeal.txd", "start": 101420583, "end": 101465639}, {"filename": "/models/gta3.img/csken.dff", "start": 101465639, "end": 101629479}, {"filename": "/models/gta3.img/csken.txd", "start": 101629479, "end": 101674535}, {"filename": "/models/gta3.img/csplay.dff", "start": 101674535, "end": 101819943}, {"filename": "/models/gta3.img/csplay.txd", "start": 101819943, "end": 101953063}, {"filename": "/models/gta3.img/cssonny.dff", "start": 101953063, "end": 102084135}, {"filename": "/models/gta3.img/cssonny.txd", "start": 102084135, "end": 102129191}, {"filename": "/models/gta3.img/deagle.ifp", "start": 102129191, "end": 102204967}, {"filename": "/models/gta3.img/deco_buildkb29_nt.dff", "start": 102204967, "end": 102237735}, {"filename": "/models/gta3.img/deco_buildkb_nt.dff", "start": 102237735, "end": 102262311}, {"filename": "/models/gta3.img/deco_polgrnda10.dff", "start": 102262311, "end": 102313511}, {"filename": "/models/gta3.img/deco_polgrnda12.dff", "start": 102313511, "end": 102450727}, {"filename": "/models/gta3.img/deco_polgrnda13.dff", "start": 102450727, "end": 102463015}, {"filename": "/models/gta3.img/delcase.dff", "start": 102463015, "end": 102471207}, {"filename": "/models/gta3.img/delcase.txd", "start": 102471207, "end": 102516263}, {"filename": "/models/gta3.img/desert_eagle.dff", "start": 102516263, "end": 102524455}, {"filename": "/models/gta3.img/desert_eagle.txd", "start": 102524455, "end": 102598183}, {"filename": "/models/gta3.img/dinghy.dff", "start": 102598183, "end": 102682151}, {"filename": "/models/gta3.img/dinghy.txd", "start": 102682151, "end": 102719015}, {"filename": "/models/gta3.img/dk_cargoshp95.dff", "start": 102719015, "end": 102919719}, {"filename": "/models/gta3.img/dk_dockroads02.dff", "start": 102919719, "end": 102948391}, {"filename": "/models/gta3.img/dk_dockroads03.dff", "start": 102948391, "end": 102977063}, {"filename": "/models/gta3.img/dk_dockroads04.dff", "start": 102977063, "end": 102991399}, {"filename": "/models/gta3.img/dk_midbuilds.txd", "start": 102991399, "end": 103454247}, {"filename": "/models/gta3.img/dk_waretank.dff", "start": 103454247, "end": 103497255}, {"filename": "/models/gta3.img/dk_waretankdoor1.dff", "start": 103497255, "end": 103499303}, {"filename": "/models/gta3.img/dkcargohull2.dff", "start": 103499303, "end": 103611943}, {"filename": "/models/gta3.img/dkcargohull2b.dff", "start": 103611943, "end": 103683623}, {"filename": "/models/gta3.img/dkcargohull2c.dff", "start": 103683623, "end": 103759399}, {"filename": "/models/gta3.img/dkcargoshp.txd", "start": 103759399, "end": 104123943}, {"filename": "/models/gta3.img/doc_crane_cab03.dff", "start": 104123943, "end": 104148519}, {"filename": "/models/gta3.img/doc_crane_cab04.dff", "start": 104148519, "end": 104195623}, {"filename": "/models/gta3.img/doc_craneeggs04.dff", "start": 104195623, "end": 104238631}, {"filename": "/models/gta3.img/doc_dockwareold.dff", "start": 104238631, "end": 104277543}, {"filename": "/models/gta3.img/doccrane.txd", "start": 104277543, "end": 104341031}, {"filename": "/models/gta3.img/docfactories.txd", "start": 104341031, "end": 104848935}, {"filename": "/models/gta3.img/dock_props01.dff", "start": 104848935, "end": 104855079}, {"filename": "/models/gta3.img/dock_props02.dff", "start": 104855079, "end": 104891943}, {"filename": "/models/gta3.img/dock_ships.txd", "start": 104891943, "end": 105137703}, {"filename": "/models/gta3.img/dockcranescale0.dff", "start": 105137703, "end": 105190951}, {"filename": "/models/gta3.img/dockfence.dff", "start": 105190951, "end": 105192999}, {"filename": "/models/gta3.img/dockfence.txd", "start": 105192999, "end": 105254439}, {"filename": "/models/gta3.img/dockfuel07.dff", "start": 105254439, "end": 105281063}, {"filename": "/models/gta3.img/dockgrass.dff", "start": 105281063, "end": 105324071}, {"filename": "/models/gta3.img/dockgrass.txd", "start": 105324071, "end": 105334311}, {"filename": "/models/gta3.img/dockgrnd.txd", "start": 105334311, "end": 105745959}, {"filename": "/models/gta3.img/docklight.txd", "start": 105745959, "end": 105786919}, {"filename": "/models/gta3.img/dockroads.txd", "start": 105786919, "end": 106026535}, {"filename": "/models/gta3.img/docks.col", "start": 106026535, "end": 106346023}, {"filename": "/models/gta3.img/docks.txd", "start": 106346023, "end": 107052583}, {"filename": "/models/gta3.img/docks10.dff", "start": 107052583, "end": 107120167}, {"filename": "/models/gta3.img/docks21.dff", "start": 107120167, "end": 107197991}, {"filename": "/models/gta3.img/docks28.dff", "start": 107197991, "end": 107240999}, {"filename": "/models/gta3.img/docks30.dff", "start": 107240999, "end": 107343399}, {"filename": "/models/gta3.img/docks31.dff", "start": 107343399, "end": 107357735}, {"filename": "/models/gta3.img/docks32.dff", "start": 107357735, "end": 107365927}, {"filename": "/models/gta3.img/docks37.dff", "start": 107365927, "end": 107398695}, {"filename": "/models/gta3.img/docks40.dff", "start": 107398695, "end": 107415079}, {"filename": "/models/gta3.img/docks62.dff", "start": 107415079, "end": 107423271}, {"filename": "/models/gta3.img/docks85.dff", "start": 107423271, "end": 107435559}, {"filename": "/models/gta3.img/docksprops04.dff", "start": 107435559, "end": 107619879}, {"filename": "/models/gta3.img/docksprops11.dff", "start": 107619879, "end": 107763239}, {"filename": "/models/gta3.img/docksprops12.dff", "start": 107763239, "end": 107906599}, {"filename": "/models/gta3.img/docksprops13.dff", "start": 107906599, "end": 108023335}, {"filename": "/models/gta3.img/docksprops14.dff", "start": 108023335, "end": 108109351}, {"filename": "/models/gta3.img/dolphin.dff", "start": 108109351, "end": 108135975}, {"filename": "/models/gta3.img/dolphin.txd", "start": 108135975, "end": 108140071}, {"filename": "/models/gta3.img/downtown.col", "start": 108140071, "end": 108645927}, {"filename": "/models/gta3.img/downtows.col", "start": 108645927, "end": 108903975}, {"filename": "/models/gta3.img/drink.dff", "start": 108903975, "end": 108908071}, {"filename": "/models/gta3.img/drink.txd", "start": 108908071, "end": 108920359}, {"filename": "/models/gta3.img/dt_blimp.txd", "start": 108920359, "end": 108924455}, {"filename": "/models/gta3.img/dt_lod.txd", "start": 108924455, "end": 109082151}, {"filename": "/models/gta3.img/dts_lodbig.txd", "start": 109082151, "end": 109125159}, {"filename": "/models/gta3.img/dts_lodsmall.txd", "start": 109125159, "end": 109153831}, {"filename": "/models/gta3.img/dts_lodsmall2.txd", "start": 109153831, "end": 109243943}, {"filename": "/models/gta3.img/dynhydrent.txd", "start": 109243943, "end": 109256231}, {"filename": "/models/gta3.img/dynsigns.txd", "start": 109256231, "end": 109282855}, {"filename": "/models/gta3.img/esperant.dff", "start": 109282855, "end": 109773699}, {"filename": "/models/gta3.img/esperant.txd", "start": 109773699, "end": 110545115}, {"filename": "/models/gta3.img/faggio.dff", "start": 110545115, "end": 110624987}, {"filename": "/models/gta3.img/faggio.txd", "start": 110624987, "end": 110735731}, {"filename": "/models/gta3.img/fire_hydrant.dff", "start": 110735731, "end": 110752115}, {"filename": "/models/gta3.img/fish1.txd", "start": 110752115, "end": 110758259}, {"filename": "/models/gta3.img/fish1single.dff", "start": 110758259, "end": 110760307}, {"filename": "/models/gta3.img/fish2.txd", "start": 110760307, "end": 110766451}, {"filename": "/models/gta3.img/fish2s.dff", "start": 110766451, "end": 110770547}, {"filename": "/models/gta3.img/fish2single.dff", "start": 110770547, "end": 110772595}, {"filename": "/models/gta3.img/fish3.txd", "start": 110772595, "end": 110778739}, {"filename": "/models/gta3.img/fish3s.dff", "start": 110778739, "end": 110780787}, {"filename": "/models/gta3.img/fish3single.dff", "start": 110780787, "end": 110782835}, {"filename": "/models/gta3.img/flatbed.dff", "start": 110782835, "end": 111281011}, {"filename": "/models/gta3.img/flatbed.txd", "start": 111281011, "end": 112077311}, {"filename": "/models/gta3.img/gda.dff", "start": 112077311, "end": 112138751}, {"filename": "/models/gta3.img/gda.txd", "start": 112138751, "end": 112183807}, {"filename": "/models/gta3.img/gdb.dff", "start": 112183807, "end": 112245247}, {"filename": "/models/gta3.img/gdb.txd", "start": 112245247, "end": 112290303}, {"filename": "/models/gta3.img/gf_lod.txd", "start": 112290303, "end": 112323071}, {"filename": "/models/gta3.img/gf_treesfw1_01.dff", "start": 112323071, "end": 112343551}, {"filename": "/models/gta3.img/gf_treesfw2_01.dff", "start": 112343551, "end": 112359935}, {"filename": "/models/gta3.img/gf_treesfw3_01.dff", "start": 112359935, "end": 112374271}, {"filename": "/models/gta3.img/gf_treesfw4_01.dff", "start": 112374271, "end": 112390655}, {"filename": "/models/gta3.img/gf_treesfw5_01.dff", "start": 112390655, "end": 112409087}, {"filename": "/models/gta3.img/ggsalonsign1.dff", "start": 112409087, "end": 112413183}, {"filename": "/models/gta3.img/glendale.dff", "start": 112413183, "end": 112909394}, {"filename": "/models/gta3.img/glendale.txd", "start": 112909394, "end": 113682986}, {"filename": "/models/gta3.img/golf.col", "start": 113682986, "end": 114045482}, {"filename": "/models/gta3.img/gr_launch.dff", "start": 114045482, "end": 114057770}, {"filename": "/models/gta3.img/gr_launch.txd", "start": 114057770, "end": 114326058}, {"filename": "/models/gta3.img/greenwoo.dff", "start": 114326058, "end": 114786214}, {"filename": "/models/gta3.img/greenwoo.txd", "start": 114786214, "end": 115531678}, {"filename": "/models/gta3.img/grenade.ifp", "start": 115531678, "end": 115552158}, {"filename": "/models/gta3.img/grenade2.dff", "start": 115552158, "end": 115556254}, {"filename": "/models/gta3.img/grenade2.txd", "start": 115556254, "end": 115625886}, {"filename": "/models/gta3.img/haiti.col", "start": 115625886, "end": 115990430}, {"filename": "/models/gta3.img/haitin.col", "start": 115990430, "end": 116320158}, {"filename": "/models/gta3.img/health.dff", "start": 116320158, "end": 116328350}, {"filename": "/models/gta3.img/hermes.dff", "start": 116328350, "end": 116878784}, {"filename": "/models/gta3.img/hermes.txd", "start": 116878784, "end": 117636536}, {"filename": "/models/gta3.img/hfost.dff", "start": 117636536, "end": 117702072}, {"filename": "/models/gta3.img/hfost.txd", "start": 117702072, "end": 117790136}, {"filename": "/models/gta3.img/hfotr.dff", "start": 117790136, "end": 117855672}, {"filename": "/models/gta3.img/hfotr.txd", "start": 117855672, "end": 117988792}, {"filename": "/models/gta3.img/hfybe.dff", "start": 117988792, "end": 118044088}, {"filename": "/models/gta3.img/hfybe.txd", "start": 118044088, "end": 118089144}, {"filename": "/models/gta3.img/hfybu.dff", "start": 118089144, "end": 118154680}, {"filename": "/models/gta3.img/hfybu.txd", "start": 118154680, "end": 118222264}, {"filename": "/models/gta3.img/hfypr.dff", "start": 118222264, "end": 118281656}, {"filename": "/models/gta3.img/hfypr.txd", "start": 118281656, "end": 118369720}, {"filename": "/models/gta3.img/hfyri.dff", "start": 118369720, "end": 118431160}, {"filename": "/models/gta3.img/hfyri.txd", "start": 118431160, "end": 118650296}, {"filename": "/models/gta3.img/hfyst.dff", "start": 118650296, "end": 118713784}, {"filename": "/models/gta3.img/hfyst.txd", "start": 118713784, "end": 118758840}, {"filename": "/models/gta3.img/hi_cutcouch.txd", "start": 118758840, "end": 118810040}, {"filename": "/models/gta3.img/hi_cuthoomintshad.txd", "start": 118810040, "end": 118967736}, {"filename": "/models/gta3.img/hi_cuthotel1.txd", "start": 118967736, "end": 119010744}, {"filename": "/models/gta3.img/hi_cuthtl2.txd", "start": 119010744, "end": 119219640}, {"filename": "/models/gta3.img/hi_cuthtl4.txd", "start": 119219640, "end": 119254456}, {"filename": "/models/gta3.img/hi_cuthtlalp.txd", "start": 119254456, "end": 119440824}, {"filename": "/models/gta3.img/hi_cutint1.txd", "start": 119440824, "end": 119573944}, {"filename": "/models/gta3.img/hi_cutintprops.txd", "start": 119573944, "end": 119674296}, {"filename": "/models/gta3.img/hi_cuttblprop.txd", "start": 119674296, "end": 119760312}, {"filename": "/models/gta3.img/hmoca.dff", "start": 119760312, "end": 119829944}, {"filename": "/models/gta3.img/hmoca.txd", "start": 119829944, "end": 119940536}, {"filename": "/models/gta3.img/hmost.dff", "start": 119940536, "end": 120016312}, {"filename": "/models/gta3.img/hmost.txd", "start": 120016312, "end": 120083896}, {"filename": "/models/gta3.img/hmotr.dff", "start": 120083896, "end": 120159672}, {"filename": "/models/gta3.img/hmotr.txd", "start": 120159672, "end": 120227256}, {"filename": "/models/gta3.img/hmyap.dff", "start": 120227256, "end": 120296888}, {"filename": "/models/gta3.img/hmyap.txd", "start": 120296888, "end": 120364472}, {"filename": "/models/gta3.img/hmyst.dff", "start": 120364472, "end": 120438200}, {"filename": "/models/gta3.img/hmyst.txd", "start": 120438200, "end": 120526264}, {"filename": "/models/gta3.img/hot_bar1_01.dff", "start": 120526264, "end": 120597944}, {"filename": "/models/gta3.img/hot_drawers1_01.dff", "start": 120597944, "end": 120636856}, {"filename": "/models/gta3.img/hot_mags1.dff", "start": 120636856, "end": 120643000}, {"filename": "/models/gta3.img/hot_mobint.txd", "start": 120643000, "end": 121343416}, {"filename": "/models/gta3.img/hot_room317.dff", "start": 121343416, "end": 121396664}, {"filename": "/models/gta3.img/hot_roomtrans.txd", "start": 121396664, "end": 121669048}, {"filename": "/models/gta3.img/hot_trans1.dff", "start": 121669048, "end": 121697720}, {"filename": "/models/gta3.img/hotel.col", "start": 121697720, "end": 121710008}, {"filename": "/models/gta3.img/hotelroomint.txd", "start": 121710008, "end": 122439096}, {"filename": "/models/gta3.img/hotroomfan.dff", "start": 122439096, "end": 122445240}, {"filename": "/models/gta3.img/hotshad1.dff", "start": 122445240, "end": 122453432}, {"filename": "/models/gta3.img/ht_doors.dff", "start": 122453432, "end": 122457528}, {"filename": "/models/gta3.img/ht_fans_nt.dff", "start": 122457528, "end": 122500536}, {"filename": "/models/gta3.img/ht_kb_couch1_nt.dff", "start": 122500536, "end": 122561976}, {"filename": "/models/gta3.img/ht_mainfloor2_nt.dff", "start": 122561976, "end": 122596792}, {"filename": "/models/gta3.img/ht_mainfloor_nt.dff", "start": 122596792, "end": 122635704}, {"filename": "/models/gta3.img/ht_upstairs.dff", "start": 122635704, "end": 122686904}, {"filename": "/models/gta3.img/ht_veg01_nt.dff", "start": 122686904, "end": 122701240}, {"filename": "/models/gta3.img/ht_veg02_dy.dff", "start": 122701240, "end": 122740152}, {"filename": "/models/gta3.img/ht_veg02_nt.dff", "start": 122740152, "end": 122779064}, {"filename": "/models/gta3.img/ht_veg04_nt.dff", "start": 122779064, "end": 122805688}, {"filename": "/models/gta3.img/htdoorhtlalp.txd", "start": 122805688, "end": 122832312}, {"filename": "/models/gta3.img/htl_dco_chair03_nt.dff", "start": 122832312, "end": 122883512}, {"filename": "/models/gta3.img/htl_ext03.txd", "start": 122883512, "end": 122901944}, {"filename": "/models/gta3.img/htl_exterior03_nt.dff", "start": 122901944, "end": 122908088}, {"filename": "/models/gta3.img/htl_gls_1_nt.dff", "start": 122908088, "end": 122910136}, {"filename": "/models/gta3.img/htl_gls_2_nt.dff", "start": 122910136, "end": 122912184}, {"filename": "/models/gta3.img/htl_gls_3_nt.dff", "start": 122912184, "end": 122914232}, {"filename": "/models/gta3.img/htl_gls_lobby.dff", "start": 122914232, "end": 122916280}, {"filename": "/models/gta3.img/htl_lftdoor1_nt.dff", "start": 122916280, "end": 122920376}, {"filename": "/models/gta3.img/htl_maintiles_nt.dff", "start": 122920376, "end": 122944952}, {"filename": "/models/gta3.img/icons3.txd", "start": 122944952, "end": 122949048}, {"filename": "/models/gta3.img/icons4.txd", "start": 122949048, "end": 122953144}, {"filename": "/models/gta3.img/icons5.txd", "start": 122953144, "end": 122959288}, {"filename": "/models/gta3.img/icons7.txd", "start": 122959288, "end": 122963384}, {"filename": "/models/gta3.img/icons9.txd", "start": 122963384, "end": 122969528}, {"filename": "/models/gta3.img/igken.dff", "start": 122969528, "end": 123047352}, {"filename": "/models/gta3.img/igken.txd", "start": 123047352, "end": 123092408}, {"filename": "/models/gta3.img/infernus.dff", "start": 123092408, "end": 123586893}, {"filename": "/models/gta3.img/infernus.txd", "start": 123586893, "end": 124533417}, {"filename": "/models/gta3.img/islandlodbeach.dff", "start": 124533417, "end": 124902057}, {"filename": "/models/gta3.img/islandlodmainland.dff", "start": 124902057, "end": 125248169}, {"filename": "/models/gta3.img/islandlolodb.txd", "start": 125248169, "end": 125334185}, {"filename": "/models/gta3.img/islandlolodm.txd", "start": 125334185, "end": 125434537}, {"filename": "/models/gta3.img/islandsf.col", "start": 125434537, "end": 125444777}, {"filename": "/models/gta3.img/jellyfish.dff", "start": 125444777, "end": 125455017}, {"filename": "/models/gta3.img/jellyfish1.txd", "start": 125455017, "end": 125465257}, {"filename": "/models/gta3.img/jfoto.dff", "start": 125465257, "end": 125530793}, {"filename": "/models/gta3.img/jfoto.txd", "start": 125530793, "end": 125598377}, {"filename": "/models/gta3.img/jmoto.dff", "start": 125598377, "end": 125659817}, {"filename": "/models/gta3.img/jmoto.txd", "start": 125659817, "end": 125704873}, {"filename": "/models/gta3.img/kaufman.dff", "start": 125704873, "end": 126525643}, {"filename": "/models/gta3.img/kaufman.txd", "start": 126525643, "end": 128056699}, {"filename": "/models/gta3.img/lamppost1.dff", "start": 128056699, "end": 128064891}, {"filename": "/models/gta3.img/lamppost2.dff", "start": 128064891, "end": 128079227}, {"filename": "/models/gta3.img/lamppost3.dff", "start": 128079227, "end": 128083323}, {"filename": "/models/gta3.img/landstal.dff", "start": 128083323, "end": 128837666}, {"filename": "/models/gta3.img/landstal.txd", "start": 128837666, "end": 129887558}, {"filename": "/models/gta3.img/lawyers.col", "start": 129887558, "end": 129903942}, {"filename": "/models/gta3.img/linerun.dff", "start": 129903942, "end": 130468747}, {"filename": "/models/gta3.img/linerun.txd", "start": 130468747, "end": 130866587}, {"filename": "/models/gta3.img/littleha.col", "start": 130866587, "end": 131243419}, {"filename": "/models/gta3.img/lod_backside.dff", "start": 131243419, "end": 131253659}, {"filename": "/models/gta3.img/lod_beacon2_nt.dff", "start": 131253659, "end": 131259803}, {"filename": "/models/gta3.img/lod_beacon_dy.dff", "start": 131259803, "end": 131267995}, {"filename": "/models/gta3.img/lod_beacon_nt.dff", "start": 131267995, "end": 131276187}, {"filename": "/models/gta3.img/lod_bridgew.dff", "start": 131276187, "end": 131278235}, {"filename": "/models/gta3.img/lod_build01.dff", "start": 131278235, "end": 131280283}, {"filename": "/models/gta3.img/lod_build02.dff", "start": 131280283, "end": 131284379}, {"filename": "/models/gta3.img/lod_build03.dff", "start": 131284379, "end": 131288475}, {"filename": "/models/gta3.img/lod_build04.dff", "start": 131288475, "end": 131290523}, {"filename": "/models/gta3.img/lod_build05.dff", "start": 131290523, "end": 131294619}, {"filename": "/models/gta3.img/lod_car_show_out.dff", "start": 131294619, "end": 131304859}, {"filename": "/models/gta3.img/lod_colony_nt.dff", "start": 131304859, "end": 131306907}, {"filename": "/models/gta3.img/lod_docks.txd", "start": 131306907, "end": 131470747}, {"filename": "/models/gta3.img/lod_docks2.txd", "start": 131470747, "end": 131507611}, {"filename": "/models/gta3.img/lod_dockwareold.dff", "start": 131507611, "end": 131511707}, {"filename": "/models/gta3.img/lod_frntstep.dff", "start": 131511707, "end": 131517851}, {"filename": "/models/gta3.img/lod_grnd2.dff", "start": 131517851, "end": 131532187}, {"filename": "/models/gta3.img/lod_grnd3.dff", "start": 131532187, "end": 131538331}, {"filename": "/models/gta3.img/lod_grnd4.dff", "start": 131538331, "end": 131550619}, {"filename": "/models/gta3.img/lod_grnd5.dff", "start": 131550619, "end": 131564955}, {"filename": "/models/gta3.img/lod_grnd6.dff", "start": 131564955, "end": 131575195}, {"filename": "/models/gta3.img/lod_grssypatch.dff", "start": 131575195, "end": 131579291}, {"filename": "/models/gta3.img/lod_hotel01.dff", "start": 131579291, "end": 131593627}, {"filename": "/models/gta3.img/lod_hotel02.dff", "start": 131593627, "end": 131614107}, {"filename": "/models/gta3.img/lod_hotel03.dff", "start": 131614107, "end": 131646875}, {"filename": "/models/gta3.img/lod_hotel04.dff", "start": 131646875, "end": 131659163}, {"filename": "/models/gta3.img/lod_hotel05.dff", "start": 131659163, "end": 131673499}, {"filename": "/models/gta3.img/lod_hotel06.dff", "start": 131673499, "end": 131687835}, {"filename": "/models/gta3.img/lod_hotel08.dff", "start": 131687835, "end": 131704219}, {"filename": "/models/gta3.img/lod_hotel13.dff", "start": 131704219, "end": 131720603}, {"filename": "/models/gta3.img/lod_hotel15.dff", "start": 131720603, "end": 131736987}, {"filename": "/models/gta3.img/lod_hotelhot_nt.dff", "start": 131736987, "end": 131739035}, {"filename": "/models/gta3.img/lod_houses27.dff", "start": 131739035, "end": 131753371}, {"filename": "/models/gta3.img/lod_htlpoolbar01.dff", "start": 131753371, "end": 131765659}, {"filename": "/models/gta3.img/lod_htlpoolbase01.dff", "start": 131765659, "end": 131769755}, {"filename": "/models/gta3.img/lod_htlpoolbase01b.dff", "start": 131769755, "end": 131773851}, {"filename": "/models/gta3.img/lod_htlpoolbase02.dff", "start": 131773851, "end": 131779995}, {"filename": "/models/gta3.img/lod_htlpoolbase03.dff", "start": 131779995, "end": 131784091}, {"filename": "/models/gta3.img/lod_htlpoolbase04.dff", "start": 131784091, "end": 131786139}, {"filename": "/models/gta3.img/lod_htlpoolrm01.dff", "start": 131786139, "end": 131792283}, {"filename": "/models/gta3.img/lod_htlpoolrm02.dff", "start": 131792283, "end": 131798427}, {"filename": "/models/gta3.img/lod_leslie_dy.dff", "start": 131798427, "end": 131802523}, {"filename": "/models/gta3.img/lod_leslie_nt.dff", "start": 131802523, "end": 131806619}, {"filename": "/models/gta3.img/lod_macalpin_dy.dff", "start": 131806619, "end": 131812763}, {"filename": "/models/gta3.img/lod_macalpin_nt.dff", "start": 131812763, "end": 131818907}, {"filename": "/models/gta3.img/lod_majest_nt.dff", "start": 131818907, "end": 131823003}, {"filename": "/models/gta3.img/lod_majestic2x_nt.dff", "start": 131823003, "end": 131827099}, {"filename": "/models/gta3.img/lod_majestic_dy.dff", "start": 131827099, "end": 131833243}, {"filename": "/models/gta3.img/lod_majestic_nt.dff", "start": 131833243, "end": 131839387}, {"filename": "/models/gta3.img/lod_majesticz_nt.dff", "start": 131839387, "end": 131841435}, {"filename": "/models/gta3.img/lod_man_grnds.txd", "start": 131841435, "end": 131894683}, {"filename": "/models/gta3.img/lod_mansion03.dff", "start": 131894683, "end": 131900827}, {"filename": "/models/gta3.img/lod_mansionbase02.dff", "start": 131900827, "end": 131904923}, {"filename": "/models/gta3.img/lod_pizzaplace.dff", "start": 131904923, "end": 131909019}, {"filename": "/models/gta3.img/lod_road01.dff", "start": 131909019, "end": 131913115}, {"filename": "/models/gta3.img/lod_road02.dff", "start": 131913115, "end": 131915163}, {"filename": "/models/gta3.img/lod_road03.dff", "start": 131915163, "end": 131919259}, {"filename": "/models/gta3.img/lod_road04.dff", "start": 131919259, "end": 131923355}, {"filename": "/models/gta3.img/lod_road05.dff", "start": 131923355, "end": 131927451}, {"filename": "/models/gta3.img/lod_road07.dff", "start": 131927451, "end": 131929499}, {"filename": "/models/gta3.img/lod_road08.dff", "start": 131929499, "end": 131931547}, {"filename": "/models/gta3.img/lod_road09.dff", "start": 131931547, "end": 131933595}, {"filename": "/models/gta3.img/lod_road10.dff", "start": 131933595, "end": 131937691}, {"filename": "/models/gta3.img/lod_road11.dff", "start": 131937691, "end": 131941787}, {"filename": "/models/gta3.img/lod_road12_gf.dff", "start": 131941787, "end": 131943835}, {"filename": "/models/gta3.img/lod_roadswsh01.dff", "start": 131943835, "end": 131949979}, {"filename": "/models/gta3.img/lod_roadswsh04.dff", "start": 131949979, "end": 131952027}, {"filename": "/models/gta3.img/lod_roadswsh05.dff", "start": 131952027, "end": 131956123}, {"filename": "/models/gta3.img/lod_roadswsh28.dff", "start": 131956123, "end": 131958171}, {"filename": "/models/gta3.img/lod_rooftopstart.dff", "start": 131958171, "end": 131964315}, {"filename": "/models/gta3.img/lod_shedbig30.dff", "start": 131964315, "end": 131970459}, {"filename": "/models/gta3.img/lod_shop.dff", "start": 131970459, "end": 131980699}, {"filename": "/models/gta3.img/lod_starbig.txd", "start": 131980699, "end": 132009371}, {"filename": "/models/gta3.img/lod_starlite_dy.dff", "start": 132009371, "end": 132011419}, {"filename": "/models/gta3.img/lod_starlite_nt.dff", "start": 132011419, "end": 132013467}, {"filename": "/models/gta3.img/lod_starsmall.txd", "start": 132013467, "end": 132076955}, {"filename": "/models/gta3.img/lod_tides2_nt.dff", "start": 132076955, "end": 132083099}, {"filename": "/models/gta3.img/lod_tides_dy.dff", "start": 132083099, "end": 132093339}, {"filename": "/models/gta3.img/lod_tides_nt.dff", "start": 132093339, "end": 132103579}, {"filename": "/models/gta3.img/lod_wtrftr_1a.dff", "start": 132103579, "end": 132105627}, {"filename": "/models/gta3.img/lodach_police.dff", "start": 132105627, "end": 132113819}, {"filename": "/models/gta3.img/lodachb.dff", "start": 132113819, "end": 132117915}, {"filename": "/models/gta3.img/lodachb1.dff", "start": 132117915, "end": 132122011}, {"filename": "/models/gta3.img/lodachbit02.dff", "start": 132122011, "end": 132126107}, {"filename": "/models/gta3.img/lodachbit03.dff", "start": 132126107, "end": 132134299}, {"filename": "/models/gta3.img/lodachbit04.dff", "start": 132134299, "end": 132148635}, {"filename": "/models/gta3.img/lodachbit04b.dff", "start": 132148635, "end": 132165019}, {"filename": "/models/gta3.img/lodachbit05.dff", "start": 132165019, "end": 132169115}, {"filename": "/models/gta3.img/lodachbit06.dff", "start": 132169115, "end": 132173211}, {"filename": "/models/gta3.img/lodachroads01.dff", "start": 132173211, "end": 132177307}, {"filename": "/models/gta3.img/lodachroads02.dff", "start": 132177307, "end": 132179355}, {"filename": "/models/gta3.img/lodachroads03.dff", "start": 132179355, "end": 132181403}, {"filename": "/models/gta3.img/lodachroads09.dff", "start": 132181403, "end": 132185499}, {"filename": "/models/gta3.img/lodachsand1.dff", "start": 132185499, "end": 132187547}, {"filename": "/models/gta3.img/lodachsand2.dff", "start": 132187547, "end": 132189595}, {"filename": "/models/gta3.img/lodachsand2b.dff", "start": 132189595, "end": 132191643}, {"filename": "/models/gta3.img/lodachwjetty01.dff", "start": 132191643, "end": 132201883}, {"filename": "/models/gta3.img/lodachwjetty02.dff", "start": 132201883, "end": 132208027}, {"filename": "/models/gta3.img/lodacne.dff", "start": 132208027, "end": 132214171}, {"filename": "/models/gta3.img/lodairpn.txd", "start": 132214171, "end": 132244891}, {"filename": "/models/gta3.img/lodallbit.dff", "start": 132244891, "end": 132246939}, {"filename": "/models/gta3.img/lodalleys1_01_nt.dff", "start": 132246939, "end": 132251035}, {"filename": "/models/gta3.img/lodalleys1b_01_dy.dff", "start": 132251035, "end": 132255131}, {"filename": "/models/gta3.img/lodalleys2_01_dy.dff", "start": 132255131, "end": 132259227}, {"filename": "/models/gta3.img/lodalleys2_01_nt.dff", "start": 132259227, "end": 132261275}, {"filename": "/models/gta3.img/lodalleys3_01_nt.dff", "start": 132261275, "end": 132263323}, {"filename": "/models/gta3.img/lodalleys3b_01_dy.dff", "start": 132263323, "end": 132265371}, {"filename": "/models/gta3.img/lodalleys3b_01_nt.dff", "start": 132265371, "end": 132267419}, {"filename": "/models/gta3.img/lodallroof01.dff", "start": 132267419, "end": 132271515}, {"filename": "/models/gta3.img/lodanahouse01.dff", "start": 132271515, "end": 132275611}, {"filename": "/models/gta3.img/lodanahouse02.dff", "start": 132275611, "end": 132279707}, {"filename": "/models/gta3.img/lodanahouse03.dff", "start": 132279707, "end": 132283803}, {"filename": "/models/gta3.img/lodanahouse04.dff", "start": 132283803, "end": 132287899}, {"filename": "/models/gta3.img/lodanahouse06.dff", "start": 132287899, "end": 132291995}, {"filename": "/models/gta3.img/lodanahouse07.dff", "start": 132291995, "end": 132296091}, {"filename": "/models/gta3.img/lodanahouse09.dff", "start": 132296091, "end": 132300187}, {"filename": "/models/gta3.img/lodanahouse10.dff", "start": 132300187, "end": 132306331}, {"filename": "/models/gta3.img/lodanahouse11.dff", "start": 132306331, "end": 132310427}, {"filename": "/models/gta3.img/lodanahouse12.dff", "start": 132310427, "end": 132330907}, {"filename": "/models/gta3.img/lodanrd1_nt.dff", "start": 132330907, "end": 132335003}, {"filename": "/models/gta3.img/lodanrda03_nt.dff", "start": 132335003, "end": 132339099}, {"filename": "/models/gta3.img/lodanrda04_nt.dff", "start": 132339099, "end": 132341147}, {"filename": "/models/gta3.img/lodanrda05_dy.dff", "start": 132341147, "end": 132343195}, {"filename": "/models/gta3.img/lodanrda05_nt.dff", "start": 132343195, "end": 132345243}, {"filename": "/models/gta3.img/lodanrda06_nt.dff", "start": 132345243, "end": 132347291}, {"filename": "/models/gta3.img/lodanroad04.dff", "start": 132347291, "end": 132353435}, {"filename": "/models/gta3.img/lodanroad05.dff", "start": 132353435, "end": 132357531}, {"filename": "/models/gta3.img/lodanrod2b_nt.dff", "start": 132357531, "end": 132361627}, {"filename": "/models/gta3.img/lodargohull2.dff", "start": 132361627, "end": 132373915}, {"filename": "/models/gta3.img/lodasket.dff", "start": 132373915, "end": 132375963}, {"filename": "/models/gta3.img/lodavehotel.dff", "start": 132375963, "end": 132396443}, {"filename": "/models/gta3.img/lodawasteb2.dff", "start": 132396443, "end": 132400539}, {"filename": "/models/gta3.img/lodawyersfront.dff", "start": 132400539, "end": 132402587}, {"filename": "/models/gta3.img/lodbackbit.dff", "start": 132402587, "end": 132406683}, {"filename": "/models/gta3.img/lodbighotel_nt.dff", "start": 132406683, "end": 132433307}, {"filename": "/models/gta3.img/lodbighotpool.dff", "start": 132433307, "end": 132439451}, {"filename": "/models/gta3.img/lodbigshops1.dff", "start": 132439451, "end": 132451739}, {"filename": "/models/gta3.img/lodbigshops2.dff", "start": 132451739, "end": 132464027}, {"filename": "/models/gta3.img/lodbillboards1.dff", "start": 132464027, "end": 132466075}, {"filename": "/models/gta3.img/lodbillboards2.dff", "start": 132466075, "end": 132468123}, {"filename": "/models/gta3.img/lodbillboards3.dff", "start": 132468123, "end": 132470171}, {"filename": "/models/gta3.img/lodbillboards4.dff", "start": 132470171, "end": 132472219}, {"filename": "/models/gta3.img/lodbillboards5.dff", "start": 132472219, "end": 132474267}, {"filename": "/models/gta3.img/lodbit02.dff", "start": 132474267, "end": 132478363}, {"filename": "/models/gta3.img/lodbit03.dff", "start": 132478363, "end": 132490651}, {"filename": "/models/gta3.img/lodboxgirdbridge.dff", "start": 132490651, "end": 132492699}, {"filename": "/models/gta3.img/lodbphq.dff", "start": 132492699, "end": 132500891}, {"filename": "/models/gta3.img/lodbridge.txd", "start": 132500891, "end": 132507035}, {"filename": "/models/gta3.img/lodbridgesupp01.dff", "start": 132507035, "end": 132517275}, {"filename": "/models/gta3.img/lodbridgesupp03.dff", "start": 132517275, "end": 132525467}, {"filename": "/models/gta3.img/lodbridgesupp04.dff", "start": 132525467, "end": 132533659}, {"filename": "/models/gta3.img/lodbuil19wall.dff", "start": 132533659, "end": 132537755}, {"filename": "/models/gta3.img/lodbuild.dff", "start": 132537755, "end": 132543899}, {"filename": "/models/gta3.img/lodbuild01.dff", "start": 132543899, "end": 132560283}, {"filename": "/models/gta3.img/lodbuilding01.dff", "start": 132560283, "end": 132572571}, {"filename": "/models/gta3.img/lodbuildingg1.dff", "start": 132572571, "end": 132582811}, {"filename": "/models/gta3.img/lodbuildingg2.dff", "start": 132582811, "end": 132591003}, {"filename": "/models/gta3.img/lodbuildingg3.dff", "start": 132591003, "end": 132599195}, {"filename": "/models/gta3.img/lodbuildingw1.dff", "start": 132599195, "end": 132607387}, {"filename": "/models/gta3.img/lodbuildkb23_nt.dff", "start": 132607387, "end": 132609435}, {"filename": "/models/gta3.img/lodbuildkb27_nt.dff", "start": 132609435, "end": 132611483}, {"filename": "/models/gta3.img/lodbuildkb2_dy.dff", "start": 132611483, "end": 132613531}, {"filename": "/models/gta3.img/lodbuildkb2_nt.dff", "start": 132613531, "end": 132615579}, {"filename": "/models/gta3.img/lodbuildws02.dff", "start": 132615579, "end": 132619675}, {"filename": "/models/gta3.img/lodbuildws03.dff", "start": 132619675, "end": 132629915}, {"filename": "/models/gta3.img/lodbuildws04.dff", "start": 132629915, "end": 132644251}, {"filename": "/models/gta3.img/lodbuildws05.dff", "start": 132644251, "end": 132648347}, {"filename": "/models/gta3.img/lodbuildws06.dff", "start": 132648347, "end": 132660635}, {"filename": "/models/gta3.img/lodbuildws07.dff", "start": 132660635, "end": 132666779}, {"filename": "/models/gta3.img/lodbuildws07.txd", "start": 132666779, "end": 132748699}, {"filename": "/models/gta3.img/lodbuildws09.dff", "start": 132748699, "end": 132760987}, {"filename": "/models/gta3.img/lodbuildws10.dff", "start": 132760987, "end": 132765083}, {"filename": "/models/gta3.img/lodbuildws12.dff", "start": 132765083, "end": 132769179}, {"filename": "/models/gta3.img/lodbuildws13.dff", "start": 132769179, "end": 132781467}, {"filename": "/models/gta3.img/lodbuildws14.dff", "start": 132781467, "end": 132795803}, {"filename": "/models/gta3.img/lodbuildws16.dff", "start": 132795803, "end": 132814235}, {"filename": "/models/gta3.img/lodbuildws17.dff", "start": 132814235, "end": 132830619}, {"filename": "/models/gta3.img/lodbuildws18.dff", "start": 132830619, "end": 132844955}, {"filename": "/models/gta3.img/lodbuildws19.dff", "start": 132844955, "end": 132855195}, {"filename": "/models/gta3.img/lodbuildws20.dff", "start": 132855195, "end": 132861339}, {"filename": "/models/gta3.img/lodbuildws21.dff", "start": 132861339, "end": 132867483}, {"filename": "/models/gta3.img/lodbuildws22.dff", "start": 132867483, "end": 132871579}, {"filename": "/models/gta3.img/lodbuildws23.dff", "start": 132871579, "end": 132877723}, {"filename": "/models/gta3.img/lodbuildws2329.dff", "start": 132877723, "end": 132881819}, {"filename": "/models/gta3.img/lodbuildws24.dff", "start": 132881819, "end": 132887963}, {"filename": "/models/gta3.img/lodbuildws25.dff", "start": 132887963, "end": 132898203}, {"filename": "/models/gta3.img/lodbuildws26.dff", "start": 132898203, "end": 132906395}, {"filename": "/models/gta3.img/lodbuildws264.dff", "start": 132906395, "end": 132908443}, {"filename": "/models/gta3.img/lodbuildws27.dff", "start": 132908443, "end": 132912539}, {"filename": "/models/gta3.img/lodbuildws28.dff", "start": 132912539, "end": 132916635}, {"filename": "/models/gta3.img/lodbuildws29.dff", "start": 132916635, "end": 132918683}, {"filename": "/models/gta3.img/lodbuildws30.dff", "start": 132918683, "end": 132922779}, {"filename": "/models/gta3.img/lodbuildws31.dff", "start": 132922779, "end": 132928923}, {"filename": "/models/gta3.img/lodbuildws312.dff", "start": 132928923, "end": 132933019}, {"filename": "/models/gta3.img/lodbuildws32.dff", "start": 132933019, "end": 132939163}, {"filename": "/models/gta3.img/lodbuildws322.dff", "start": 132939163, "end": 132943259}, {"filename": "/models/gta3.img/lodbuildws33.dff", "start": 132943259, "end": 132947355}, {"filename": "/models/gta3.img/lodbuildws332.dff", "start": 132947355, "end": 132951451}, {"filename": "/models/gta3.img/lodbuildws35.dff", "start": 132951451, "end": 132955547}, {"filename": "/models/gta3.img/lodbuildws36.dff", "start": 132955547, "end": 132963739}, {"filename": "/models/gta3.img/lodbuildws40.dff", "start": 132963739, "end": 132967835}, {"filename": "/models/gta3.img/lodbuildws41.dff", "start": 132967835, "end": 132973979}, {"filename": "/models/gta3.img/lodbuildws42.dff", "start": 132973979, "end": 132978075}, {"filename": "/models/gta3.img/lodbuildws43.dff", "start": 132978075, "end": 132986267}, {"filename": "/models/gta3.img/lodbuildws43ins.dff", "start": 132986267, "end": 132992411}, {"filename": "/models/gta3.img/lodbuildws44.dff", "start": 132992411, "end": 132996507}, {"filename": "/models/gta3.img/lodbuildws45.dff", "start": 132996507, "end": 133000603}, {"filename": "/models/gta3.img/lodcargoshp03.dff", "start": 133000603, "end": 133002651}, {"filename": "/models/gta3.img/lodcargoshp04.dff", "start": 133002651, "end": 133004699}, {"filename": "/models/gta3.img/lodcargoshp05.dff", "start": 133004699, "end": 133016987}, {"filename": "/models/gta3.img/lodcargoshp24.dff", "start": 133016987, "end": 133019035}, {"filename": "/models/gta3.img/lodcargoshp25.dff", "start": 133019035, "end": 133021083}, {"filename": "/models/gta3.img/lodcargoshp28.dff", "start": 133021083, "end": 133029275}, {"filename": "/models/gta3.img/lodcargoshp32.dff", "start": 133029275, "end": 133035419}, {"filename": "/models/gta3.img/lodcargoshp35.dff", "start": 133035419, "end": 133039515}, {"filename": "/models/gta3.img/lodcargoshp40.dff", "start": 133039515, "end": 133043611}, {"filename": "/models/gta3.img/lodcargoshp41.dff", "start": 133043611, "end": 133049755}, {"filename": "/models/gta3.img/lodcargoshp47.dff", "start": 133049755, "end": 133053851}, {"filename": "/models/gta3.img/lodcargoshp50.dff", "start": 133053851, "end": 133057947}, {"filename": "/models/gta3.img/lodcargoshp51.dff", "start": 133057947, "end": 133062043}, {"filename": "/models/gta3.img/lodcargoshp64.dff", "start": 133062043, "end": 133078427}, {"filename": "/models/gta3.img/lodcargoshp70.dff", "start": 133078427, "end": 133086619}, {"filename": "/models/gta3.img/lodcargoshp71.dff", "start": 133086619, "end": 133094811}, {"filename": "/models/gta3.img/lodcargoshp72.dff", "start": 133094811, "end": 133100955}, {"filename": "/models/gta3.img/lodcargoshp73.dff", "start": 133100955, "end": 133107099}, {"filename": "/models/gta3.img/lodcarparkterm1.dff", "start": 133107099, "end": 133109147}, {"filename": "/models/gta3.img/lodcasagrande_nt.dff", "start": 133109147, "end": 133115291}, {"filename": "/models/gta3.img/lodcbtwbrdge.dff", "start": 133115291, "end": 133119387}, {"filename": "/models/gta3.img/lodchariot.dff", "start": 133119387, "end": 133146011}, {"filename": "/models/gta3.img/lodchbt.dff", "start": 133146011, "end": 133152155}, {"filename": "/models/gta3.img/lodchbtb.dff", "start": 133152155, "end": 133154203}, {"filename": "/models/gta3.img/lodchbtb2.dff", "start": 133154203, "end": 133158299}, {"filename": "/models/gta3.img/lodchlo1.dff", "start": 133158299, "end": 133160347}, {"filename": "/models/gta3.img/lodcland01.dff", "start": 133160347, "end": 133166491}, {"filename": "/models/gta3.img/lodcland03.dff", "start": 133166491, "end": 133170587}, {"filename": "/models/gta3.img/lodcland09.dff", "start": 133170587, "end": 133176731}, {"filename": "/models/gta3.img/lodcland10.dff", "start": 133176731, "end": 133180827}, {"filename": "/models/gta3.img/lodcland11.dff", "start": 133180827, "end": 133182875}, {"filename": "/models/gta3.img/lodcland12.dff", "start": 133182875, "end": 133195163}, {"filename": "/models/gta3.img/lodcland13_gf.dff", "start": 133195163, "end": 133201307}, {"filename": "/models/gta3.img/lodclevelander_nt.dff", "start": 133201307, "end": 133209499}, {"filename": "/models/gta3.img/lodclubback.dff", "start": 133209499, "end": 133211547}, {"filename": "/models/gta3.img/lodclubhouse1.dff", "start": 133211547, "end": 133217691}, {"filename": "/models/gta3.img/lodclubout_nt.dff", "start": 133217691, "end": 133232027}, {"filename": "/models/gta3.img/lodcoast.dff", "start": 133232027, "end": 133236123}, {"filename": "/models/gta3.img/lodcoast04.dff", "start": 133236123, "end": 133242267}, {"filename": "/models/gta3.img/lodcoast06.dff", "start": 133242267, "end": 133248411}, {"filename": "/models/gta3.img/lodcoast08.dff", "start": 133248411, "end": 133252507}, {"filename": "/models/gta3.img/lodcoast1.dff", "start": 133252507, "end": 133256603}, {"filename": "/models/gta3.img/lodcoast2.dff", "start": 133256603, "end": 133260699}, {"filename": "/models/gta3.img/lodcoast3.dff", "start": 133260699, "end": 133264795}, {"filename": "/models/gta3.img/lodcoast4.dff", "start": 133264795, "end": 133268891}, {"filename": "/models/gta3.img/lodcoast5.dff", "start": 133268891, "end": 133270939}, {"filename": "/models/gta3.img/lodcoasta2.dff", "start": 133270939, "end": 133272987}, {"filename": "/models/gta3.img/lodcoastfirst.dff", "start": 133272987, "end": 133277083}, {"filename": "/models/gta3.img/lodcolony2_dy.dff", "start": 133277083, "end": 133281179}, {"filename": "/models/gta3.img/lodcolony2_nt.dff", "start": 133281179, "end": 133285275}, {"filename": "/models/gta3.img/lodcolony_nt.dff", "start": 133285275, "end": 133291419}, {"filename": "/models/gta3.img/lodconcwall1.dff", "start": 133291419, "end": 133293467}, {"filename": "/models/gta3.img/lodcpaynspray.dff", "start": 133293467, "end": 133299611}, {"filename": "/models/gta3.img/lodcrushbn.dff", "start": 133299611, "end": 133305755}, {"filename": "/models/gta3.img/lodcrushefuck.dff", "start": 133305755, "end": 133322139}, {"filename": "/models/gta3.img/lodcrushers08.dff", "start": 133322139, "end": 133338523}, {"filename": "/models/gta3.img/lodcrushers10.dff", "start": 133338523, "end": 133352859}, {"filename": "/models/gta3.img/lodcrushers12.dff", "start": 133352859, "end": 133361051}, {"filename": "/models/gta3.img/lodcrushers14.dff", "start": 133361051, "end": 133371291}, {"filename": "/models/gta3.img/lodcrushju.dff", "start": 133371291, "end": 133383579}, {"filename": "/models/gta3.img/lodcrushmnf.dff", "start": 133383579, "end": 133389723}, {"filename": "/models/gta3.img/lodd_build2.dff", "start": 133389723, "end": 133404059}, {"filename": "/models/gta3.img/lodd_buildnew.dff", "start": 133404059, "end": 133408155}, {"filename": "/models/gta3.img/lodd_downtown05.dff", "start": 133408155, "end": 133410203}, {"filename": "/models/gta3.img/lodd_downtown06.dff", "start": 133410203, "end": 133412251}, {"filename": "/models/gta3.img/lodd_downtown08.dff", "start": 133412251, "end": 133414299}, {"filename": "/models/gta3.img/lodd_downtown09.dff", "start": 133414299, "end": 133416347}, {"filename": "/models/gta3.img/lodd_downtown10.dff", "start": 133416347, "end": 133418395}, {"filename": "/models/gta3.img/lodd_downtown_new1.dff", "start": 133418395, "end": 133420443}, {"filename": "/models/gta3.img/loddbridge.dff", "start": 133420443, "end": 133426587}, {"filename": "/models/gta3.img/loddingsite1.dff", "start": 133426587, "end": 133449115}, {"filename": "/models/gta3.img/loddockbridge.dff", "start": 133449115, "end": 133453211}, {"filename": "/models/gta3.img/loddockroads01.dff", "start": 133453211, "end": 133455259}, {"filename": "/models/gta3.img/loddockroads02.dff", "start": 133455259, "end": 133459355}, {"filename": "/models/gta3.img/loddockroads03.dff", "start": 133459355, "end": 133463451}, {"filename": "/models/gta3.img/loddockroads04.dff", "start": 133463451, "end": 133465499}, {"filename": "/models/gta3.img/loddockroads05.dff", "start": 133465499, "end": 133467547}, {"filename": "/models/gta3.img/loddockroads06.dff", "start": 133467547, "end": 133471643}, {"filename": "/models/gta3.img/loddockroads08.dff", "start": 133471643, "end": 133475739}, {"filename": "/models/gta3.img/loddockwall1.dff", "start": 133475739, "end": 133477787}, {"filename": "/models/gta3.img/loder2.dff", "start": 133477787, "end": 133481883}, {"filename": "/models/gta3.img/lodfiretrucks.dff", "start": 133481883, "end": 133488027}, {"filename": "/models/gta3.img/lodflargetank1.dff", "start": 133488027, "end": 133494171}, {"filename": "/models/gta3.img/lodflyingschool1.dff", "start": 133494171, "end": 133498267}, {"filename": "/models/gta3.img/lodfreightterminal1.dff", "start": 133498267, "end": 133506459}, {"filename": "/models/gta3.img/lodfroad1.dff", "start": 133506459, "end": 133508507}, {"filename": "/models/gta3.img/lodfroad2.dff", "start": 133508507, "end": 133512603}, {"filename": "/models/gta3.img/lodfroad3.dff", "start": 133512603, "end": 133514651}, {"filename": "/models/gta3.img/lodfroad4.dff", "start": 133514651, "end": 133516699}, {"filename": "/models/gta3.img/lodfroad5.dff", "start": 133516699, "end": 133520795}, {"filename": "/models/gta3.img/lodfsthus.dff", "start": 133520795, "end": 133533083}, {"filename": "/models/gta3.img/lodgarage02.dff", "start": 133533083, "end": 133535131}, {"filename": "/models/gta3.img/lodgarage03.dff", "start": 133535131, "end": 133539227}, {"filename": "/models/gta3.img/lodgarage1.dff", "start": 133539227, "end": 133541275}, {"filename": "/models/gta3.img/lodgarden1.dff", "start": 133541275, "end": 133547419}, {"filename": "/models/gta3.img/lodgate03.dff", "start": 133547419, "end": 133553563}, {"filename": "/models/gta3.img/lodgate1.dff", "start": 133553563, "end": 133555611}, {"filename": "/models/gta3.img/lodgate2.dff", "start": 133555611, "end": 133557659}, {"filename": "/models/gta3.img/lodgatesa.dff", "start": 133557659, "end": 133569947}, {"filename": "/models/gta3.img/lodgatesb.dff", "start": 133569947, "end": 133582235}, {"filename": "/models/gta3.img/lodgolfwall.dff", "start": 133582235, "end": 133592475}, {"filename": "/models/gta3.img/lodgrnda1.dff", "start": 133592475, "end": 133594523}, {"filename": "/models/gta3.img/lodground1.dff", "start": 133594523, "end": 133598619}, {"filename": "/models/gta3.img/lodground2.dff", "start": 133598619, "end": 133606811}, {"filename": "/models/gta3.img/lodground3.dff", "start": 133606811, "end": 133610907}, {"filename": "/models/gta3.img/lodground4.dff", "start": 133610907, "end": 133615003}, {"filename": "/models/gta3.img/lodground5.dff", "start": 133615003, "end": 133619099}, {"filename": "/models/gta3.img/lodgroundplane.dff", "start": 133619099, "end": 133623195}, {"filename": "/models/gta3.img/lodgstoreext.dff", "start": 133623195, "end": 133627291}, {"filename": "/models/gta3.img/lodh_ammu.dff", "start": 133627291, "end": 133629339}, {"filename": "/models/gta3.img/lodh_brdgsup1.dff", "start": 133629339, "end": 133637531}, {"filename": "/models/gta3.img/lodh_deco01.dff", "start": 133637531, "end": 133658011}, {"filename": "/models/gta3.img/lodh_deco02.dff", "start": 133658011, "end": 133674395}, {"filename": "/models/gta3.img/lodh_deco03.dff", "start": 133674395, "end": 133692827}, {"filename": "/models/gta3.img/lodh_deco04.dff", "start": 133692827, "end": 133705115}, {"filename": "/models/gta3.img/lodh_deco05.dff", "start": 133705115, "end": 133711259}, {"filename": "/models/gta3.img/lodh_hardwares.dff", "start": 133711259, "end": 133713307}, {"filename": "/models/gta3.img/lodh_hosp02.dff", "start": 133713307, "end": 133719451}, {"filename": "/models/gta3.img/lodh_hosp03.dff", "start": 133719451, "end": 133725595}, {"filename": "/models/gta3.img/lodh_hospgrnd.dff", "start": 133725595, "end": 133731739}, {"filename": "/models/gta3.img/lodh_hospital.dff", "start": 133731739, "end": 133741979}, {"filename": "/models/gta3.img/lodh_pizzaplace.dff", "start": 133741979, "end": 133744027}, {"filename": "/models/gta3.img/lodhafinrd.dff", "start": 133744027, "end": 133748123}, {"filename": "/models/gta3.img/lodhahousebk2.dff", "start": 133748123, "end": 133754267}, {"filename": "/models/gta3.img/lodhaiblockc1.dff", "start": 133754267, "end": 133766555}, {"filename": "/models/gta3.img/lodhaiblockc2.dff", "start": 133766555, "end": 133778843}, {"filename": "/models/gta3.img/lodhaiblockc3.dff", "start": 133778843, "end": 133791131}, {"filename": "/models/gta3.img/lodhaiti.txd", "start": 133791131, "end": 133873051}, {"filename": "/models/gta3.img/lodhaitibig.txd", "start": 133873051, "end": 133922203}, {"filename": "/models/gta3.img/lodhaitin.txd", "start": 133922203, "end": 133944731}, {"filename": "/models/gta3.img/lodhaitinbig.txd", "start": 133944731, "end": 134073755}, {"filename": "/models/gta3.img/lodhangar1.dff", "start": 134073755, "end": 134077851}, {"filename": "/models/gta3.img/lodhangar1_01.dff", "start": 134077851, "end": 134081947}, {"filename": "/models/gta3.img/lodhangar1_04.dff", "start": 134081947, "end": 134086043}, {"filename": "/models/gta3.img/lodharoada2.dff", "start": 134086043, "end": 134090139}, {"filename": "/models/gta3.img/lodharoada3.dff", "start": 134090139, "end": 134096283}, {"filename": "/models/gta3.img/lodharoada4.dff", "start": 134096283, "end": 134098331}, {"filename": "/models/gta3.img/lodharoada6.dff", "start": 134098331, "end": 134102427}, {"filename": "/models/gta3.img/lodharoada8.dff", "start": 134102427, "end": 134106523}, {"filename": "/models/gta3.img/lodharoada9.dff", "start": 134106523, "end": 134110619}, {"filename": "/models/gta3.img/lodhavabit01.dff", "start": 134110619, "end": 134116763}, {"filename": "/models/gta3.img/lodhavabit04.dff", "start": 134116763, "end": 134118811}, {"filename": "/models/gta3.img/lodhavabit05.dff", "start": 134118811, "end": 134122907}, {"filename": "/models/gta3.img/lodhavabit06.dff", "start": 134122907, "end": 134127003}, {"filename": "/models/gta3.img/lodhavabit07.dff", "start": 134127003, "end": 134131099}, {"filename": "/models/gta3.img/lodhavabit08.dff", "start": 134131099, "end": 134135195}, {"filename": "/models/gta3.img/lodhavabit09.dff", "start": 134135195, "end": 134139291}, {"filename": "/models/gta3.img/lodhavabit10.dff", "start": 134139291, "end": 134143387}, {"filename": "/models/gta3.img/lodhavabit11.dff", "start": 134143387, "end": 134147483}, {"filename": "/models/gta3.img/lodhavabit12.dff", "start": 134147483, "end": 134151579}, {"filename": "/models/gta3.img/lodhavana.txd", "start": 134151579, "end": 134237595}, {"filename": "/models/gta3.img/lodhavanabrg.txd", "start": 134237595, "end": 134241691}, {"filename": "/models/gta3.img/lodhavbig.txd", "start": 134241691, "end": 134268315}, {"filename": "/models/gta3.img/lodhavroad3.dff", "start": 134268315, "end": 134272411}, {"filename": "/models/gta3.img/lodhawaste1.dff", "start": 134272411, "end": 134276507}, {"filename": "/models/gta3.img/lodhawaste2.dff", "start": 134276507, "end": 134278555}, {"filename": "/models/gta3.img/lodhbit04bgrd1.dff", "start": 134278555, "end": 134282651}, {"filename": "/models/gta3.img/lodhbit04grd2.dff", "start": 134282651, "end": 134284699}, {"filename": "/models/gta3.img/lodhbtjetty01.dff", "start": 134284699, "end": 134294939}, {"filename": "/models/gta3.img/lodhbtjetty02.dff", "start": 134294939, "end": 134305179}, {"filename": "/models/gta3.img/lodhbuild003.dff", "start": 134305179, "end": 134307227}, {"filename": "/models/gta3.img/lodhbuild005.dff", "start": 134307227, "end": 134313371}, {"filename": "/models/gta3.img/lodhbuild013.dff", "start": 134313371, "end": 134317467}, {"filename": "/models/gta3.img/lodhbuild014.dff", "start": 134317467, "end": 134321563}, {"filename": "/models/gta3.img/lodhbuild015.dff", "start": 134321563, "end": 134325659}, {"filename": "/models/gta3.img/lodhbuild017.dff", "start": 134325659, "end": 134333851}, {"filename": "/models/gta3.img/lodhbuild018.dff", "start": 134333851, "end": 134339995}, {"filename": "/models/gta3.img/lodhbuild020.dff", "start": 134339995, "end": 134344091}, {"filename": "/models/gta3.img/lodhbuild021.dff", "start": 134344091, "end": 134346139}, {"filename": "/models/gta3.img/lodhbuild025.dff", "start": 134346139, "end": 134352283}, {"filename": "/models/gta3.img/lodhbuild033.dff", "start": 134352283, "end": 134360475}, {"filename": "/models/gta3.img/lodhbuild040.dff", "start": 134360475, "end": 134366619}, {"filename": "/models/gta3.img/lodhbuild043.dff", "start": 134366619, "end": 134391195}, {"filename": "/models/gta3.img/lodhbuild050.dff", "start": 134391195, "end": 134399387}, {"filename": "/models/gta3.img/lodhbuild058.dff", "start": 134399387, "end": 134409627}, {"filename": "/models/gta3.img/lodhbuild062.dff", "start": 134409627, "end": 134415771}, {"filename": "/models/gta3.img/lodhbuild063.dff", "start": 134415771, "end": 134426011}, {"filename": "/models/gta3.img/lodhbuild066.dff", "start": 134426011, "end": 134436251}, {"filename": "/models/gta3.img/lodhbuild068.dff", "start": 134436251, "end": 134442395}, {"filename": "/models/gta3.img/lodhbuild069.dff", "start": 134442395, "end": 134448539}, {"filename": "/models/gta3.img/lodhbuild071.dff", "start": 134448539, "end": 134450587}, {"filename": "/models/gta3.img/lodhbuild072.dff", "start": 134450587, "end": 134452635}, {"filename": "/models/gta3.img/lodhbuild073.dff", "start": 134452635, "end": 134454683}, {"filename": "/models/gta3.img/lodhbuild074.dff", "start": 134454683, "end": 134456731}, {"filename": "/models/gta3.img/lodhbuild075.dff", "start": 134456731, "end": 134458779}, {"filename": "/models/gta3.img/lodhbuild081.dff", "start": 134458779, "end": 134460827}, {"filename": "/models/gta3.img/lodhbuild0812.dff", "start": 134460827, "end": 134464923}, {"filename": "/models/gta3.img/lodhbuild111.dff", "start": 134464923, "end": 134466971}, {"filename": "/models/gta3.img/lodhbuild112.dff", "start": 134466971, "end": 134475163}, {"filename": "/models/gta3.img/lodhbuild114.dff", "start": 134475163, "end": 134477211}, {"filename": "/models/gta3.img/lodhbuild116.dff", "start": 134477211, "end": 134479259}, {"filename": "/models/gta3.img/lodhbuild119.dff", "start": 134479259, "end": 134481307}, {"filename": "/models/gta3.img/lodhbuild120.dff", "start": 134481307, "end": 134483355}, {"filename": "/models/gta3.img/lodhbuild121.dff", "start": 134483355, "end": 134485403}, {"filename": "/models/gta3.img/lodhbuild184.dff", "start": 134485403, "end": 134495643}, {"filename": "/models/gta3.img/lodhbuild187.dff", "start": 134495643, "end": 134503835}, {"filename": "/models/gta3.img/lodhbuild192.dff", "start": 134503835, "end": 134505883}, {"filename": "/models/gta3.img/lodhbuild195.dff", "start": 134505883, "end": 134507931}, {"filename": "/models/gta3.img/lodhbuild198.dff", "start": 134507931, "end": 134516123}, {"filename": "/models/gta3.img/lodhbuild203.dff", "start": 134516123, "end": 134520219}, {"filename": "/models/gta3.img/lodhbuild213.dff", "start": 134520219, "end": 134522267}, {"filename": "/models/gta3.img/lodhbuild214.dff", "start": 134522267, "end": 134524315}, {"filename": "/models/gta3.img/lodhbuild215.dff", "start": 134524315, "end": 134526363}, {"filename": "/models/gta3.img/lodhbuild216.dff", "start": 134526363, "end": 134528411}, {"filename": "/models/gta3.img/lodhipment01.dff", "start": 134528411, "end": 134532507}, {"filename": "/models/gta3.img/lodhipment03.dff", "start": 134532507, "end": 134540699}, {"filename": "/models/gta3.img/lodhipment05.dff", "start": 134540699, "end": 134548891}, {"filename": "/models/gta3.img/lodhit14.dff", "start": 134548891, "end": 134557083}, {"filename": "/models/gta3.img/lodhitbuild1a.dff", "start": 134557083, "end": 134569371}, {"filename": "/models/gta3.img/lodhitbuild1aa.dff", "start": 134569371, "end": 134575515}, {"filename": "/models/gta3.img/lodhitsjm.dff", "start": 134575515, "end": 134587803}, {"filename": "/models/gta3.img/lodhland_01.dff", "start": 134587803, "end": 134595995}, {"filename": "/models/gta3.img/lodhnewsky1.dff", "start": 134595995, "end": 134606235}, {"filename": "/models/gta3.img/lodhnewsky2.dff", "start": 134606235, "end": 134614427}, {"filename": "/models/gta3.img/lodhnewsky3.dff", "start": 134614427, "end": 134624667}, {"filename": "/models/gta3.img/lodhookerinn1.dff", "start": 134624667, "end": 134630811}, {"filename": "/models/gta3.img/lodhoose.dff", "start": 134630811, "end": 134645147}, {"filename": "/models/gta3.img/lodhotel.dff", "start": 134645147, "end": 134649243}, {"filename": "/models/gta3.img/lodhotel10.dff", "start": 134649243, "end": 134669723}, {"filename": "/models/gta3.img/lodhotel_windows.dff", "start": 134669723, "end": 134682011}, {"filename": "/models/gta3.img/lodhotelgrnd.dff", "start": 134682011, "end": 134688155}, {"filename": "/models/gta3.img/lodhpaynspray.dff", "start": 134688155, "end": 134694299}, {"filename": "/models/gta3.img/lodhpshoutdet.dff", "start": 134694299, "end": 134704539}, {"filename": "/models/gta3.img/lodhsebuild1.dff", "start": 134704539, "end": 134710683}, {"filename": "/models/gta3.img/lodhtooceanrd1.dff", "start": 134710683, "end": 134714779}, {"filename": "/models/gta3.img/lodhundermall1.dff", "start": 134714779, "end": 134720923}, {"filename": "/models/gta3.img/lodhwbridge.dff", "start": 134720923, "end": 134733211}, {"filename": "/models/gta3.img/lodi_exterior_front.dff", "start": 134733211, "end": 134735259}, {"filename": "/models/gta3.img/lodiamiland049.dff", "start": 134735259, "end": 134739355}, {"filename": "/models/gta3.img/lodiamiland050.dff", "start": 134739355, "end": 134743451}, {"filename": "/models/gta3.img/lodiamiland057.dff", "start": 134743451, "end": 134747547}, {"filename": "/models/gta3.img/lodicecream01.dff", "start": 134747547, "end": 134755739}, {"filename": "/models/gta3.img/lodighsandgrs1.dff", "start": 134755739, "end": 134759835}, {"filename": "/models/gta3.img/lodigste1mesh.dff", "start": 134759835, "end": 134761883}, {"filename": "/models/gta3.img/lodina1.dff", "start": 134761883, "end": 134768027}, {"filename": "/models/gta3.img/lodina2.dff", "start": 134768027, "end": 134774171}, {"filename": "/models/gta3.img/lodina3.dff", "start": 134774171, "end": 134776219}, {"filename": "/models/gta3.img/lodinhaiti.dff", "start": 134776219, "end": 134782363}, {"filename": "/models/gta3.img/lodipad0.dff", "start": 134782363, "end": 134786459}, {"filename": "/models/gta3.img/lodita1.dff", "start": 134786459, "end": 134790555}, {"filename": "/models/gta3.img/loditcut.dff", "start": 134790555, "end": 134800795}, {"filename": "/models/gta3.img/loditnewbt.dff", "start": 134800795, "end": 134819227}, {"filename": "/models/gta3.img/loditsnip.dff", "start": 134819227, "end": 134825371}, {"filename": "/models/gta3.img/loditwarhus.dff", "start": 134825371, "end": 134833563}, {"filename": "/models/gta3.img/lodjumbo_01.dff", "start": 134833563, "end": 134841755}, {"filename": "/models/gta3.img/lodk_camjones.dff", "start": 134841755, "end": 134847899}, {"filename": "/models/gta3.img/lodk_grassarea.dff", "start": 134847899, "end": 134854043}, {"filename": "/models/gta3.img/lodkcranescale0.dff", "start": 134854043, "end": 134870427}, {"filename": "/models/gta3.img/lodkcranescale01.dff", "start": 134870427, "end": 134886811}, {"filename": "/models/gta3.img/lodkfuel02.dff", "start": 134886811, "end": 134899099}, {"filename": "/models/gta3.img/lodkhus.dff", "start": 134899099, "end": 134911387}, {"filename": "/models/gta3.img/lodks10.dff", "start": 134911387, "end": 134929819}, {"filename": "/models/gta3.img/lodks21.dff", "start": 134929819, "end": 134950299}, {"filename": "/models/gta3.img/lodks28.dff", "start": 134950299, "end": 134956443}, {"filename": "/models/gta3.img/lodks29.dff", "start": 134956443, "end": 134958491}, {"filename": "/models/gta3.img/lodks30.dff", "start": 134958491, "end": 134976923}, {"filename": "/models/gta3.img/lodks31.dff", "start": 134976923, "end": 134978971}, {"filename": "/models/gta3.img/lodks32.dff", "start": 134978971, "end": 134981019}, {"filename": "/models/gta3.img/lodks37.dff", "start": 134981019, "end": 134989211}, {"filename": "/models/gta3.img/lodks40.dff", "start": 134989211, "end": 134993307}, {"filename": "/models/gta3.img/lodks42.dff", "start": 134993307, "end": 135001499}, {"filename": "/models/gta3.img/lodks43.dff", "start": 135001499, "end": 135009691}, {"filename": "/models/gta3.img/lodks46.dff", "start": 135009691, "end": 135028123}, {"filename": "/models/gta3.img/lodks47.dff", "start": 135028123, "end": 135036315}, {"filename": "/models/gta3.img/lodks48.dff", "start": 135036315, "end": 135044507}, {"filename": "/models/gta3.img/lodks49.dff", "start": 135044507, "end": 135052699}, {"filename": "/models/gta3.img/lodks50.dff", "start": 135052699, "end": 135058843}, {"filename": "/models/gta3.img/lodks51.dff", "start": 135058843, "end": 135067035}, {"filename": "/models/gta3.img/lodks52.dff", "start": 135067035, "end": 135073179}, {"filename": "/models/gta3.img/lodks53.dff", "start": 135073179, "end": 135081371}, {"filename": "/models/gta3.img/lodks60.dff", "start": 135081371, "end": 135085467}, {"filename": "/models/gta3.img/lodks61.dff", "start": 135085467, "end": 135091611}, {"filename": "/models/gta3.img/lodks62.dff", "start": 135091611, "end": 135095707}, {"filename": "/models/gta3.img/lodks85.dff", "start": 135095707, "end": 135099803}, {"filename": "/models/gta3.img/lodks92.dff", "start": 135099803, "end": 135103899}, {"filename": "/models/gta3.img/lodks93.dff", "start": 135103899, "end": 135107995}, {"filename": "/models/gta3.img/lodks95.dff", "start": 135107995, "end": 135114139}, {"filename": "/models/gta3.img/lodks96.dff", "start": 135114139, "end": 135118235}, {"filename": "/models/gta3.img/lodksprops04.dff", "start": 135118235, "end": 135122331}, {"filename": "/models/gta3.img/lodksprops10.dff", "start": 135122331, "end": 135128475}, {"filename": "/models/gta3.img/lodksprops11.dff", "start": 135128475, "end": 135134619}, {"filename": "/models/gta3.img/lodksprops14.dff", "start": 135134619, "end": 135138715}, {"filename": "/models/gta3.img/lodksprops15.dff", "start": 135138715, "end": 135140763}, {"filename": "/models/gta3.img/lodksware01.dff", "start": 135140763, "end": 135151003}, {"filename": "/models/gta3.img/lodl2.dff", "start": 135151003, "end": 135155099}, {"filename": "/models/gta3.img/lodlargebuild.dff", "start": 135155099, "end": 135161243}, {"filename": "/models/gta3.img/lodld1.dff", "start": 135161243, "end": 135169435}, {"filename": "/models/gta3.img/lodldingground40.dff", "start": 135169435, "end": 135175579}, {"filename": "/models/gta3.img/lodldingground50.dff", "start": 135175579, "end": 135181723}, {"filename": "/models/gta3.img/lodldingsite2.dff", "start": 135181723, "end": 135189915}, {"filename": "/models/gta3.img/lodlhaitibuilding02.dff", "start": 135189915, "end": 135200155}, {"filename": "/models/gta3.img/lodlhaitibuilding06.dff", "start": 135200155, "end": 135212443}, {"filename": "/models/gta3.img/lodlhaitibuilding07.dff", "start": 135212443, "end": 135214491}, {"filename": "/models/gta3.img/lodlhaitibuilding09.dff", "start": 135214491, "end": 135228827}, {"filename": "/models/gta3.img/lodlhland.dff", "start": 135228827, "end": 135239067}, {"filename": "/models/gta3.img/lodlightext.dff", "start": 135239067, "end": 135249307}, {"filename": "/models/gta3.img/lodlleygb_nt.dff", "start": 135249307, "end": 135251355}, {"filename": "/models/gta3.img/lodlleyground.dff", "start": 135251355, "end": 135253403}, {"filename": "/models/gta3.img/lodlleyground2.dff", "start": 135253403, "end": 135255451}, {"filename": "/models/gta3.img/lodlleyground4.dff", "start": 135255451, "end": 135257499}, {"filename": "/models/gta3.img/lodlleyground5.dff", "start": 135257499, "end": 135259547}, {"filename": "/models/gta3.img/lodmain_body.dff", "start": 135259547, "end": 135277979}, {"filename": "/models/gta3.img/lodmainisl2b_01.dff", "start": 135277979, "end": 135282075}, {"filename": "/models/gta3.img/lodmainisl4_01.dff", "start": 135282075, "end": 135292315}, {"filename": "/models/gta3.img/lodmainisl5_01.dff", "start": 135292315, "end": 135304603}, {"filename": "/models/gta3.img/lodmainisland1a_01.dff", "start": 135304603, "end": 135310747}, {"filename": "/models/gta3.img/lodmainisland1b_01.dff", "start": 135310747, "end": 135318939}, {"filename": "/models/gta3.img/lodmainisland2a_01.dff", "start": 135318939, "end": 135325083}, {"filename": "/models/gta3.img/lodmainisland3a_01.dff", "start": 135325083, "end": 135337371}, {"filename": "/models/gta3.img/lodmainisland3b_01.dff", "start": 135337371, "end": 135345563}, {"filename": "/models/gta3.img/lodmainisland6a_01.dff", "start": 135345563, "end": 135353755}, {"filename": "/models/gta3.img/lodmainisland6b_01.dff", "start": 135353755, "end": 135357851}, {"filename": "/models/gta3.img/lodmajestic2c_dy.dff", "start": 135357851, "end": 135363995}, {"filename": "/models/gta3.img/lodmajestic2c_nt.dff", "start": 135363995, "end": 135372187}, {"filename": "/models/gta3.img/lodmalsign4.dff", "start": 135372187, "end": 135374235}, {"filename": "/models/gta3.img/lodmalsign5.dff", "start": 135374235, "end": 135376283}, {"filename": "/models/gta3.img/lodmiamilanda.dff", "start": 135376283, "end": 135382427}, {"filename": "/models/gta3.img/lodmiland027a.dff", "start": 135382427, "end": 135394715}, {"filename": "/models/gta3.img/lodmiland037.dff", "start": 135394715, "end": 135400859}, {"filename": "/models/gta3.img/lodmiland039.dff", "start": 135400859, "end": 135402907}, {"filename": "/models/gta3.img/lodmiland041.dff", "start": 135402907, "end": 135407003}, {"filename": "/models/gta3.img/lodmiland170.dff", "start": 135407003, "end": 135411099}, {"filename": "/models/gta3.img/lodmiland171.dff", "start": 135411099, "end": 135423387}, {"filename": "/models/gta3.img/lodmiland172.dff", "start": 135423387, "end": 135431579}, {"filename": "/models/gta3.img/lodmiland173.dff", "start": 135431579, "end": 135441819}, {"filename": "/models/gta3.img/lodmiland174.dff", "start": 135441819, "end": 135450011}, {"filename": "/models/gta3.img/lodmiland175.dff", "start": 135450011, "end": 135456155}, {"filename": "/models/gta3.img/lodmiland176.dff", "start": 135456155, "end": 135458203}, {"filename": "/models/gta3.img/lodmiland177.dff", "start": 135458203, "end": 135460251}, {"filename": "/models/gta3.img/lodmiland178.dff", "start": 135460251, "end": 135462299}, {"filename": "/models/gta3.img/lodmiland179.dff", "start": 135462299, "end": 135472539}, {"filename": "/models/gta3.img/lodmiland180.dff", "start": 135472539, "end": 135480731}, {"filename": "/models/gta3.img/lodmiland_kb01_nt.dff", "start": 135480731, "end": 135486875}, {"filename": "/models/gta3.img/lodmiland_kb02.dff", "start": 135486875, "end": 135490971}, {"filename": "/models/gta3.img/lodmiland_kb03.dff", "start": 135490971, "end": 135495067}, {"filename": "/models/gta3.img/lodmiland_kb04.dff", "start": 135495067, "end": 135499163}, {"filename": "/models/gta3.img/lodmiland_kb10.dff", "start": 135499163, "end": 135511451}, {"filename": "/models/gta3.img/lodmiland_kb10b.dff", "start": 135511451, "end": 135515547}, {"filename": "/models/gta3.img/lodmiland_kb11.dff", "start": 135515547, "end": 135521691}, {"filename": "/models/gta3.img/lodmiland_kb11b.dff", "start": 135521691, "end": 135525787}, {"filename": "/models/gta3.img/lodmiland_kb12.dff", "start": 135525787, "end": 135531931}, {"filename": "/models/gta3.img/lodmiland_kb12b.dff", "start": 135531931, "end": 135536027}, {"filename": "/models/gta3.img/lodmiland_kb13.dff", "start": 135536027, "end": 135544219}, {"filename": "/models/gta3.img/lodmiland_ws04.dff", "start": 135544219, "end": 135548315}, {"filename": "/models/gta3.img/lodmiland_ws04b.dff", "start": 135548315, "end": 135552411}, {"filename": "/models/gta3.img/lodmiodnroadxa.dff", "start": 135552411, "end": 135556507}, {"filename": "/models/gta3.img/lodmrgbase1.dff", "start": 135556507, "end": 135564699}, {"filename": "/models/gta3.img/lodmrgbuild1.dff", "start": 135564699, "end": 135572891}, {"filename": "/models/gta3.img/lodmrgbuild2.dff", "start": 135572891, "end": 135579035}, {"filename": "/models/gta3.img/lodmrgtrees4.dff", "start": 135579035, "end": 135593371}, {"filename": "/models/gta3.img/lodnbbridge1.dff", "start": 135593371, "end": 135609755}, {"filename": "/models/gta3.img/lodnbchw.txd", "start": 135609755, "end": 135699867}, {"filename": "/models/gta3.img/lodnbeach.txd", "start": 135699867, "end": 135847323}, {"filename": "/models/gta3.img/lodnbeach07.dff", "start": 135847323, "end": 135849371}, {"filename": "/models/gta3.img/lodnbeach08.dff", "start": 135849371, "end": 135851419}, {"filename": "/models/gta3.img/lodnbeachbtbig.txd", "start": 135851419, "end": 135921051}, {"filename": "/models/gta3.img/lodnbeachwbig.txd", "start": 135921051, "end": 135962011}, {"filename": "/models/gta3.img/lodnbwroad01.dff", "start": 135962011, "end": 135964059}, {"filename": "/models/gta3.img/lodnbwroad02.dff", "start": 135964059, "end": 135968155}, {"filename": "/models/gta3.img/lodnbwroad03.dff", "start": 135968155, "end": 135970203}, {"filename": "/models/gta3.img/lodnbwroad04.dff", "start": 135970203, "end": 135972251}, {"filename": "/models/gta3.img/lodnbwroad05.dff", "start": 135972251, "end": 135974299}, {"filename": "/models/gta3.img/lodnbwroad06.dff", "start": 135974299, "end": 135976347}, {"filename": "/models/gta3.img/lodnbwroad07.dff", "start": 135976347, "end": 135980443}, {"filename": "/models/gta3.img/lodnbwroad08.dff", "start": 135980443, "end": 135992731}, {"filename": "/models/gta3.img/lodnd04.dff", "start": 135992731, "end": 135994779}, {"filename": "/models/gta3.img/lodnebasea0.dff", "start": 135994779, "end": 135998875}, {"filename": "/models/gta3.img/lodnetopa0.dff", "start": 135998875, "end": 136002971}, {"filename": "/models/gta3.img/lodnewscafe_nt.dff", "start": 136002971, "end": 136009115}, {"filename": "/models/gta3.img/lodngst2mesh.dff", "start": 136009115, "end": 136011163}, {"filename": "/models/gta3.img/lodnorthbridge1.dff", "start": 136011163, "end": 136017307}, {"filename": "/models/gta3.img/lodnorthisland1.dff", "start": 136017307, "end": 136023451}, {"filename": "/models/gta3.img/lodnorthisland2.dff", "start": 136023451, "end": 136031643}, {"filename": "/models/gta3.img/lodnorthisland3.dff", "start": 136031643, "end": 136037787}, {"filename": "/models/gta3.img/lodnorthstar.dff", "start": 136037787, "end": 136064411}, {"filename": "/models/gta3.img/lodnrthroad01.dff", "start": 136064411, "end": 136066459}, {"filename": "/models/gta3.img/lodnrthroad02.dff", "start": 136066459, "end": 136068507}, {"filename": "/models/gta3.img/lodnrthroad03.dff", "start": 136068507, "end": 136070555}, {"filename": "/models/gta3.img/lodnrthroad04.dff", "start": 136070555, "end": 136074651}, {"filename": "/models/gta3.img/lodnrthroad05.dff", "start": 136074651, "end": 136076699}, {"filename": "/models/gta3.img/lodnrthroad06.dff", "start": 136076699, "end": 136078747}, {"filename": "/models/gta3.img/lodnrthroad07.dff", "start": 136078747, "end": 136080795}, {"filename": "/models/gta3.img/lodnrthroad08.dff", "start": 136080795, "end": 136082843}, {"filename": "/models/gta3.img/lodnrthroad09.dff", "start": 136082843, "end": 136084891}, {"filename": "/models/gta3.img/lodnrthroad10.dff", "start": 136084891, "end": 136086939}, {"filename": "/models/gta3.img/lodnrthroad11.dff", "start": 136086939, "end": 136088987}, {"filename": "/models/gta3.img/lodnrthroad12.dff", "start": 136088987, "end": 136091035}, {"filename": "/models/gta3.img/lodnrthroad13.dff", "start": 136091035, "end": 136093083}, {"filename": "/models/gta3.img/lodnrthroad14.dff", "start": 136093083, "end": 136095131}, {"filename": "/models/gta3.img/lodnrthroad15.dff", "start": 136095131, "end": 136097179}, {"filename": "/models/gta3.img/lodnrthroad16.dff", "start": 136097179, "end": 136099227}, {"filename": "/models/gta3.img/lodnrthroad17.dff", "start": 136099227, "end": 136101275}, {"filename": "/models/gta3.img/lodnrthroad18.dff", "start": 136101275, "end": 136103323}, {"filename": "/models/gta3.img/lodnrthroad19.dff", "start": 136103323, "end": 136105371}, {"filename": "/models/gta3.img/lodnrthroad20.dff", "start": 136105371, "end": 136107419}, {"filename": "/models/gta3.img/lodnrthroad21.dff", "start": 136107419, "end": 136109467}, {"filename": "/models/gta3.img/lodnrthroad22.dff", "start": 136109467, "end": 136111515}, {"filename": "/models/gta3.img/lodnrthroad23.dff", "start": 136111515, "end": 136113563}, {"filename": "/models/gta3.img/lodnrthroad24.dff", "start": 136113563, "end": 136115611}, {"filename": "/models/gta3.img/lodnrthroad25.dff", "start": 136115611, "end": 136117659}, {"filename": "/models/gta3.img/lodnrthroad26.dff", "start": 136117659, "end": 136119707}, {"filename": "/models/gta3.img/lodnrthroad27.dff", "start": 136119707, "end": 136121755}, {"filename": "/models/gta3.img/lodnrthroad28.dff", "start": 136121755, "end": 136123803}, {"filename": "/models/gta3.img/lodntoon03.dff", "start": 136123803, "end": 136136091}, {"filename": "/models/gta3.img/lodntoon04.dff", "start": 136136091, "end": 136146331}, {"filename": "/models/gta3.img/lodntoon13.dff", "start": 136146331, "end": 136156571}, {"filename": "/models/gta3.img/lodntoon18.dff", "start": 136156571, "end": 136164763}, {"filename": "/models/gta3.img/lodntoon20.dff", "start": 136164763, "end": 136172955}, {"filename": "/models/gta3.img/lodntoon21.dff", "start": 136172955, "end": 136179099}, {"filename": "/models/gta3.img/lodntoon22.dff", "start": 136179099, "end": 136199579}, {"filename": "/models/gta3.img/lodntoon23.dff", "start": 136199579, "end": 136220059}, {"filename": "/models/gta3.img/lodntoon32.dff", "start": 136220059, "end": 136232347}, {"filename": "/models/gta3.img/lodntoon34.dff", "start": 136232347, "end": 136236443}, {"filename": "/models/gta3.img/lodntoon39.dff", "start": 136236443, "end": 136256923}, {"filename": "/models/gta3.img/lodntoon49.dff", "start": 136256923, "end": 136261019}, {"filename": "/models/gta3.img/lodntoon53.dff", "start": 136261019, "end": 136279451}, {"filename": "/models/gta3.img/lodntoon54.dff", "start": 136279451, "end": 136287643}, {"filename": "/models/gta3.img/lodntoon55.dff", "start": 136287643, "end": 136306075}, {"filename": "/models/gta3.img/lodntoon56.dff", "start": 136306075, "end": 136324507}, {"filename": "/models/gta3.img/lodntoon65.dff", "start": 136324507, "end": 136326555}, {"filename": "/models/gta3.img/lodntoon66.dff", "start": 136326555, "end": 136328603}, {"filename": "/models/gta3.img/lodntoon70.dff", "start": 136328603, "end": 136332699}, {"filename": "/models/gta3.img/lodntoon72.dff", "start": 136332699, "end": 136336795}, {"filename": "/models/gta3.img/lodntoon72b.dff", "start": 136336795, "end": 136338843}, {"filename": "/models/gta3.img/lodntoon73.dff", "start": 136338843, "end": 136340891}, {"filename": "/models/gta3.img/lodntoon73b.dff", "start": 136340891, "end": 136342939}, {"filename": "/models/gta3.img/lodntoon74.dff", "start": 136342939, "end": 136344987}, {"filename": "/models/gta3.img/lodntoon_newbit.dff", "start": 136344987, "end": 136353179}, {"filename": "/models/gta3.img/lodntoon_shops.dff", "start": 136353179, "end": 136361371}, {"filename": "/models/gta3.img/lodntoon_top02.dff", "start": 136361371, "end": 136369563}, {"filename": "/models/gta3.img/lodntoon_top04.dff", "start": 136369563, "end": 136371611}, {"filename": "/models/gta3.img/lodntoon_top05.dff", "start": 136371611, "end": 136373659}, {"filename": "/models/gta3.img/lodntoon_top07.dff", "start": 136373659, "end": 136377755}, {"filename": "/models/gta3.img/lodntoon_top08.dff", "start": 136377755, "end": 136394139}, {"filename": "/models/gta3.img/lodntoon_top10.dff", "start": 136394139, "end": 136406427}, {"filename": "/models/gta3.img/lodo_buildkb09.dff", "start": 136406427, "end": 136408475}, {"filename": "/models/gta3.img/lodo_buildkb25.dff", "start": 136408475, "end": 136412571}, {"filename": "/models/gta3.img/lodo_buildkb29_nt.dff", "start": 136412571, "end": 136414619}, {"filename": "/models/gta3.img/lodo_buildkb_nt.dff", "start": 136414619, "end": 136416667}, {"filename": "/models/gta3.img/lodo_copgrnd.dff", "start": 136416667, "end": 136420763}, {"filename": "/models/gta3.img/lodo_polbuild.dff", "start": 136420763, "end": 136435099}, {"filename": "/models/gta3.img/lodo_polgrnda10.dff", "start": 136435099, "end": 136447387}, {"filename": "/models/gta3.img/lodo_polgrnda12.dff", "start": 136447387, "end": 136461723}, {"filename": "/models/gta3.img/lodo_polgrnda13.dff", "start": 136461723, "end": 136467867}, {"filename": "/models/gta3.img/lodo_polgrnda14.dff", "start": 136467867, "end": 136476059}, {"filename": "/models/gta3.img/lodo_polgrnda16.dff", "start": 136476059, "end": 136484251}, {"filename": "/models/gta3.img/lodo_polgrnda17.dff", "start": 136484251, "end": 136492443}, {"filename": "/models/gta3.img/lodo_polgrnda7.dff", "start": 136492443, "end": 136504731}, {"filename": "/models/gta3.img/lodo_polgrnda8.dff", "start": 136504731, "end": 136510875}, {"filename": "/models/gta3.img/lodo_polgrnda9.dff", "start": 136510875, "end": 136525211}, {"filename": "/models/gta3.img/lodoada1.dff", "start": 136525211, "end": 136529307}, {"filename": "/models/gta3.img/lodoadc1.dff", "start": 136529307, "end": 136531355}, {"filename": "/models/gta3.img/lodoadc2.dff", "start": 136531355, "end": 136533403}, {"filename": "/models/gta3.img/lodoadc3.dff", "start": 136533403, "end": 136535451}, {"filename": "/models/gta3.img/lodoadc4.dff", "start": 136535451, "end": 136537499}, {"filename": "/models/gta3.img/lodoastsky1.dff", "start": 136537499, "end": 136541595}, {"filename": "/models/gta3.img/lodoastsky2.dff", "start": 136541595, "end": 136545691}, {"filename": "/models/gta3.img/lodoceanrdn1_dy.dff", "start": 136545691, "end": 136547739}, {"filename": "/models/gta3.img/lodoceanrdn1_nt.dff", "start": 136547739, "end": 136549787}, {"filename": "/models/gta3.img/lododrive.txd", "start": 136549787, "end": 136660379}, {"filename": "/models/gta3.img/lododriven.txd", "start": 136660379, "end": 136801691}, {"filename": "/models/gta3.img/lodoshp010.dff", "start": 136801691, "end": 136818075}, {"filename": "/models/gta3.img/lodotel01.dff", "start": 136818075, "end": 136828315}, {"filename": "/models/gta3.img/lodotel02.dff", "start": 136828315, "end": 136836507}, {"filename": "/models/gta3.img/lodotel03.dff", "start": 136836507, "end": 136859035}, {"filename": "/models/gta3.img/lodowsand1.dff", "start": 136859035, "end": 136861083}, {"filename": "/models/gta3.img/lodowsand2.dff", "start": 136861083, "end": 136863131}, {"filename": "/models/gta3.img/lodpad_grdn_2.dff", "start": 136863131, "end": 136875419}, {"filename": "/models/gta3.img/lodpaynspray.dff", "start": 136875419, "end": 136879515}, {"filename": "/models/gta3.img/lodpedbridge3.dff", "start": 136879515, "end": 136885659}, {"filename": "/models/gta3.img/lodpelican2_nt.dff", "start": 136885659, "end": 136889755}, {"filename": "/models/gta3.img/lodpelican_dy.dff", "start": 136889755, "end": 136893851}, {"filename": "/models/gta3.img/lodpelican_nt.dff", "start": 136893851, "end": 136897947}, {"filename": "/models/gta3.img/lodphils.dff", "start": 136897947, "end": 136920475}, {"filename": "/models/gta3.img/lodpital.dff", "start": 136920475, "end": 136928667}, {"filename": "/models/gta3.img/lodpizzaplace.dff", "start": 136928667, "end": 136932763}, {"filename": "/models/gta3.img/lodport_carpark0.dff", "start": 136932763, "end": 136934811}, {"filename": "/models/gta3.img/lodprintworks.dff", "start": 136934811, "end": 136947099}, {"filename": "/models/gta3.img/lodracecotop.dff", "start": 136947099, "end": 136965531}, {"filename": "/models/gta3.img/lodracecotop2.dff", "start": 136965531, "end": 136975771}, {"filename": "/models/gta3.img/lodrbits05.dff", "start": 136975771, "end": 136983963}, {"filename": "/models/gta3.img/lodrbits06.dff", "start": 136983963, "end": 136988059}, {"filename": "/models/gta3.img/lodrbits13.dff", "start": 136988059, "end": 136992155}, {"filename": "/models/gta3.img/lodrbits13b.dff", "start": 136992155, "end": 136994203}, {"filename": "/models/gta3.img/lodrbits13c.dff", "start": 136994203, "end": 136996251}, {"filename": "/models/gta3.img/lodrbits17.dff", "start": 136996251, "end": 137000347}, {"filename": "/models/gta3.img/lodrbits22.dff", "start": 137000347, "end": 137004443}, {"filename": "/models/gta3.img/lodrbits24.dff", "start": 137004443, "end": 137008539}, {"filename": "/models/gta3.img/lodrbits45.dff", "start": 137008539, "end": 137012635}, {"filename": "/models/gta3.img/lodrbits46.dff", "start": 137012635, "end": 137018779}, {"filename": "/models/gta3.img/lodrbits49.dff", "start": 137018779, "end": 137022875}, {"filename": "/models/gta3.img/lodrbits50.dff", "start": 137022875, "end": 137026971}, {"filename": "/models/gta3.img/lodrbits51.dff", "start": 137026971, "end": 137031067}, {"filename": "/models/gta3.img/lodrbits52.dff", "start": 137031067, "end": 137037211}, {"filename": "/models/gta3.img/lodrbits54.dff", "start": 137037211, "end": 137043355}, {"filename": "/models/gta3.img/lodrbits55.dff", "start": 137043355, "end": 137047451}, {"filename": "/models/gta3.img/lodrbits56.dff", "start": 137047451, "end": 137053595}, {"filename": "/models/gta3.img/lodrbits57.dff", "start": 137053595, "end": 137057691}, {"filename": "/models/gta3.img/lodrbits58.dff", "start": 137057691, "end": 137063835}, {"filename": "/models/gta3.img/lodrbits59.dff", "start": 137063835, "end": 137069979}, {"filename": "/models/gta3.img/lodrbl02b.dff", "start": 137069979, "end": 137072027}, {"filename": "/models/gta3.img/lodrblocks02.dff", "start": 137072027, "end": 137082267}, {"filename": "/models/gta3.img/lodrblocks03.dff", "start": 137082267, "end": 137090459}, {"filename": "/models/gta3.img/lodrblocks04.dff", "start": 137090459, "end": 137106843}, {"filename": "/models/gta3.img/lodrbridge2.dff", "start": 137106843, "end": 137131419}, {"filename": "/models/gta3.img/lodrdepot.dff", "start": 137131419, "end": 137139611}, {"filename": "/models/gta3.img/lodrentrance02.dff", "start": 137139611, "end": 137145755}, {"filename": "/models/gta3.img/lodrentrance1.dff", "start": 137145755, "end": 137151899}, {"filename": "/models/gta3.img/lodrhouse.dff", "start": 137151899, "end": 137158043}, {"filename": "/models/gta3.img/lodrisland_road1.dff", "start": 137158043, "end": 137162139}, {"filename": "/models/gta3.img/lodrisland_road2.dff", "start": 137162139, "end": 137166235}, {"filename": "/models/gta3.img/lodrisland_road3.dff", "start": 137166235, "end": 137168283}, {"filename": "/models/gta3.img/lodrisland_road4.dff", "start": 137168283, "end": 137172379}, {"filename": "/models/gta3.img/lodrisland_road6.dff", "start": 137172379, "end": 137174427}, {"filename": "/models/gta3.img/lodrland01.dff", "start": 137174427, "end": 137186715}, {"filename": "/models/gta3.img/lodrland02.dff", "start": 137186715, "end": 137194907}, {"filename": "/models/gta3.img/lodrland03.dff", "start": 137194907, "end": 137201051}, {"filename": "/models/gta3.img/lodrland04.dff", "start": 137201051, "end": 137205147}, {"filename": "/models/gta3.img/lodrland05.dff", "start": 137205147, "end": 137211291}, {"filename": "/models/gta3.img/lodrland06.dff", "start": 137211291, "end": 137221531}, {"filename": "/models/gta3.img/lodrland07.dff", "start": 137221531, "end": 137225627}, {"filename": "/models/gta3.img/lodrland08.dff", "start": 137225627, "end": 137231771}, {"filename": "/models/gta3.img/lodrland10.dff", "start": 137231771, "end": 137237915}, {"filename": "/models/gta3.img/lodrlandnew9.dff", "start": 137237915, "end": 137248155}, {"filename": "/models/gta3.img/lodrnda1.dff", "start": 137248155, "end": 137250203}, {"filename": "/models/gta3.img/lodroad01_nt.dff", "start": 137250203, "end": 137254299}, {"filename": "/models/gta3.img/lodroad03od.dff", "start": 137254299, "end": 137256347}, {"filename": "/models/gta3.img/lodroad04od_nt.dff", "start": 137256347, "end": 137260443}, {"filename": "/models/gta3.img/lodroad05od.dff", "start": 137260443, "end": 137264539}, {"filename": "/models/gta3.img/lodroad08_nt.dff", "start": 137264539, "end": 137266587}, {"filename": "/models/gta3.img/lodroadab.dff", "start": 137266587, "end": 137268635}, {"filename": "/models/gta3.img/lodroadnew.dff", "start": 137268635, "end": 137272731}, {"filename": "/models/gta3.img/lodroadsect1.dff", "start": 137272731, "end": 137278875}, {"filename": "/models/gta3.img/lodroadsect2a.dff", "start": 137278875, "end": 137285019}, {"filename": "/models/gta3.img/lodroadsect2b.dff", "start": 137285019, "end": 137291163}, {"filename": "/models/gta3.img/lodroadsect3.dff", "start": 137291163, "end": 137307547}, {"filename": "/models/gta3.img/lodroadsect4.dff", "start": 137307547, "end": 137328027}, {"filename": "/models/gta3.img/lodroadsect5.dff", "start": 137328027, "end": 137338267}, {"filename": "/models/gta3.img/lodroadsigns1_02.dff", "start": 137338267, "end": 137342363}, {"filename": "/models/gta3.img/lodroadsigns1_03.dff", "start": 137342363, "end": 137346459}, {"filename": "/models/gta3.img/lodroadx.dff", "start": 137346459, "end": 137350555}, {"filename": "/models/gta3.img/lodrtrees1.dff", "start": 137350555, "end": 137364891}, {"filename": "/models/gta3.img/lodrtrees2.dff", "start": 137364891, "end": 137375131}, {"filename": "/models/gta3.img/lodrtrees3.dff", "start": 137375131, "end": 137389467}, {"filename": "/models/gta3.img/lodrtrees4.dff", "start": 137389467, "end": 137403803}, {"filename": "/models/gta3.img/lodrtrees5.dff", "start": 137403803, "end": 137416091}, {"filename": "/models/gta3.img/lodrwaterfrnt05.dff", "start": 137416091, "end": 137422235}, {"filename": "/models/gta3.img/lodrwaterfrnt06.dff", "start": 137422235, "end": 137424283}, {"filename": "/models/gta3.img/lodrwaterfrnt06b.dff", "start": 137424283, "end": 137428379}, {"filename": "/models/gta3.img/lodrwaterfrnt07.dff", "start": 137428379, "end": 137430427}, {"filename": "/models/gta3.img/lodrwaterfrnt07b.dff", "start": 137430427, "end": 137432475}, {"filename": "/models/gta3.img/lodrwaterfrnt08.dff", "start": 137432475, "end": 137436571}, {"filename": "/models/gta3.img/lodrwaterfrnt09.dff", "start": 137436571, "end": 137440667}, {"filename": "/models/gta3.img/lodrwaterfrnt09b.dff", "start": 137440667, "end": 137442715}, {"filename": "/models/gta3.img/lodrwaterfrnt10.dff", "start": 137442715, "end": 137446811}, {"filename": "/models/gta3.img/lodrwaterfrnt11.dff", "start": 137446811, "end": 137450907}, {"filename": "/models/gta3.img/lodrwaterfrnt12.dff", "start": 137450907, "end": 137455003}, {"filename": "/models/gta3.img/lodsbrgestart_r_dt.dff", "start": 137455003, "end": 137471387}, {"filename": "/models/gta3.img/lodsbrgestart_r_nt.dff", "start": 137471387, "end": 137487771}, {"filename": "/models/gta3.img/lodsbrgestartf_dt.dff", "start": 137487771, "end": 137500059}, {"filename": "/models/gta3.img/lodsbrgestartf_nt.dff", "start": 137500059, "end": 137512347}, {"filename": "/models/gta3.img/lodsbridge07_dt.dff", "start": 137512347, "end": 137520539}, {"filename": "/models/gta3.img/lodsbridge07_nt.dff", "start": 137520539, "end": 137528731}, {"filename": "/models/gta3.img/lodsbridge1_dt.dff", "start": 137528731, "end": 137536923}, {"filename": "/models/gta3.img/lodsbridge1_nt.dff", "start": 137536923, "end": 137545115}, {"filename": "/models/gta3.img/lodsbridge2_dt.dff", "start": 137545115, "end": 137557403}, {"filename": "/models/gta3.img/lodsbridge2_nt.dff", "start": 137557403, "end": 137569691}, {"filename": "/models/gta3.img/lodsbridge3_dt.dff", "start": 137569691, "end": 137577883}, {"filename": "/models/gta3.img/lodsbridge3_nt.dff", "start": 137577883, "end": 137586075}, {"filename": "/models/gta3.img/lodsbridge4_dt.dff", "start": 137586075, "end": 137596315}, {"filename": "/models/gta3.img/lodsbridge4_nt.dff", "start": 137596315, "end": 137606555}, {"filename": "/models/gta3.img/lodsbridge5_dt.dff", "start": 137606555, "end": 137620891}, {"filename": "/models/gta3.img/lodsbridge5_nt.dff", "start": 137620891, "end": 137635227}, {"filename": "/models/gta3.img/lodsbridge6_dt.dff", "start": 137635227, "end": 137643419}, {"filename": "/models/gta3.img/lodsbridge6_nt.dff", "start": 137643419, "end": 137651611}, {"filename": "/models/gta3.img/lodsbridge_dt.dff", "start": 137651611, "end": 137663899}, {"filename": "/models/gta3.img/lodsbridge_nt.dff", "start": 137663899, "end": 137674139}, {"filename": "/models/gta3.img/lodscarland226.dff", "start": 137674139, "end": 137676187}, {"filename": "/models/gta3.img/lodse_ext.dff", "start": 137676187, "end": 137680283}, {"filename": "/models/gta3.img/lodse_pier.dff", "start": 137680283, "end": 137688475}, {"filename": "/models/gta3.img/lodse_pierfence.dff", "start": 137688475, "end": 137692571}, {"filename": "/models/gta3.img/lodseaplaland1.dff", "start": 137692571, "end": 137694619}, {"filename": "/models/gta3.img/lodseaplanehanger1.dff", "start": 137694619, "end": 137700763}, {"filename": "/models/gta3.img/lodsection.dff", "start": 137700763, "end": 137706907}, {"filename": "/models/gta3.img/lodsho_ext1.dff", "start": 137706907, "end": 137711003}, {"filename": "/models/gta3.img/lodshopfronts01.dff", "start": 137711003, "end": 137721243}, {"filename": "/models/gta3.img/lodshopfronts01b.dff", "start": 137721243, "end": 137733531}, {"filename": "/models/gta3.img/lodshopfronts02.dff", "start": 137733531, "end": 137739675}, {"filename": "/models/gta3.img/lodshopfronts02b.dff", "start": 137739675, "end": 137747867}, {"filename": "/models/gta3.img/lodshpfrnts1grd.dff", "start": 137747867, "end": 137751963}, {"filename": "/models/gta3.img/lodshpfrts1bgrd.dff", "start": 137751963, "end": 137754011}, {"filename": "/models/gta3.img/lodsion1_a.dff", "start": 137754011, "end": 137762203}, {"filename": "/models/gta3.img/lodsion1_base.dff", "start": 137762203, "end": 137774491}, {"filename": "/models/gta3.img/lodsion1b.dff", "start": 137774491, "end": 137782683}, {"filename": "/models/gta3.img/lodsion1gardens.dff", "start": 137782683, "end": 137803163}, {"filename": "/models/gta3.img/lodsion2_a.dff", "start": 137803163, "end": 137809307}, {"filename": "/models/gta3.img/lodsion2_b.dff", "start": 137809307, "end": 137815451}, {"filename": "/models/gta3.img/lodsion2c.dff", "start": 137815451, "end": 137821595}, {"filename": "/models/gta3.img/lodsion3_b.dff", "start": 137821595, "end": 137827739}, {"filename": "/models/gta3.img/lodsion3_base.dff", "start": 137827739, "end": 137848219}, {"filename": "/models/gta3.img/lodsion3_c.dff", "start": 137848219, "end": 137854363}, {"filename": "/models/gta3.img/lodsion3a.dff", "start": 137854363, "end": 137866651}, {"filename": "/models/gta3.img/lodsion3gardens.dff", "start": 137866651, "end": 137885083}, {"filename": "/models/gta3.img/lodsjmcrush.dff", "start": 137885083, "end": 137895323}, {"filename": "/models/gta3.img/lodsmahangar2_01.dff", "start": 137895323, "end": 137899419}, {"filename": "/models/gta3.img/lodsmahangar2_02.dff", "start": 137899419, "end": 137903515}, {"filename": "/models/gta3.img/lodsmahangar2_03.dff", "start": 137903515, "end": 137907611}, {"filename": "/models/gta3.img/lodsmahangar2_04.dff", "start": 137907611, "end": 137911707}, {"filename": "/models/gta3.img/lodsmahangar2_05.dff", "start": 137911707, "end": 137915803}, {"filename": "/models/gta3.img/lodsmahangar2_06.dff", "start": 137915803, "end": 137919899}, {"filename": "/models/gta3.img/lodsmallhangars1_01.dff", "start": 137919899, "end": 137926043}, {"filename": "/models/gta3.img/lodsmallhangars1_02.dff", "start": 137926043, "end": 137930139}, {"filename": "/models/gta3.img/lodsmallhangars1_03.dff", "start": 137930139, "end": 137934235}, {"filename": "/models/gta3.img/lodsmallisland1_01.dff", "start": 137934235, "end": 137936283}, {"filename": "/models/gta3.img/lodsmallislebridge1.dff", "start": 137936283, "end": 137940379}, {"filename": "/models/gta3.img/lodsmallradar1.dff", "start": 137940379, "end": 137946523}, {"filename": "/models/gta3.img/lodsmallradar1_02.dff", "start": 137946523, "end": 137952667}, {"filename": "/models/gta3.img/lodst_woodfence30.dff", "start": 137952667, "end": 137954715}, {"filename": "/models/gta3.img/lodstadiumland_a.dff", "start": 137954715, "end": 137967003}, {"filename": "/models/gta3.img/lodstadiumland_b.dff", "start": 137967003, "end": 137981339}, {"filename": "/models/gta3.img/lodstadiumland_c.dff", "start": 137981339, "end": 137989531}, {"filename": "/models/gta3.img/lodstadiumland_d.dff", "start": 137989531, "end": 137995675}, {"filename": "/models/gta3.img/lodsthroad01.dff", "start": 137995675, "end": 137997723}, {"filename": "/models/gta3.img/lodsthroad02.dff", "start": 137997723, "end": 137999771}, {"filename": "/models/gta3.img/lodsthroad03.dff", "start": 137999771, "end": 138001819}, {"filename": "/models/gta3.img/lodsthroad04.dff", "start": 138001819, "end": 138003867}, {"filename": "/models/gta3.img/lodsthroad05.dff", "start": 138003867, "end": 138005915}, {"filename": "/models/gta3.img/lodsthroad06.dff", "start": 138005915, "end": 138007963}, {"filename": "/models/gta3.img/lodsthroad07.dff", "start": 138007963, "end": 138010011}, {"filename": "/models/gta3.img/lodsthroad08.dff", "start": 138010011, "end": 138012059}, {"filename": "/models/gta3.img/lodsthroad09.dff", "start": 138012059, "end": 138014107}, {"filename": "/models/gta3.img/lodsthroad10.dff", "start": 138014107, "end": 138016155}, {"filename": "/models/gta3.img/lodsthroad11.dff", "start": 138016155, "end": 138018203}, {"filename": "/models/gta3.img/lodsthroad12.dff", "start": 138018203, "end": 138020251}, {"filename": "/models/gta3.img/lodsthroad13.dff", "start": 138020251, "end": 138022299}, {"filename": "/models/gta3.img/lodsthroad14.dff", "start": 138022299, "end": 138024347}, {"filename": "/models/gta3.img/lodsthroad15.dff", "start": 138024347, "end": 138026395}, {"filename": "/models/gta3.img/lodsthroad16.dff", "start": 138026395, "end": 138028443}, {"filename": "/models/gta3.img/lodsthroad17.dff", "start": 138028443, "end": 138030491}, {"filename": "/models/gta3.img/lodsthroad18.dff", "start": 138030491, "end": 138032539}, {"filename": "/models/gta3.img/lodsthroad19.dff", "start": 138032539, "end": 138034587}, {"filename": "/models/gta3.img/lodsthroad20.dff", "start": 138034587, "end": 138036635}, {"filename": "/models/gta3.img/lodsthroad21.dff", "start": 138036635, "end": 138038683}, {"filename": "/models/gta3.img/lodsthroad22.dff", "start": 138038683, "end": 138040731}, {"filename": "/models/gta3.img/lodsthroad23.dff", "start": 138040731, "end": 138042779}, {"filename": "/models/gta3.img/lodsthroad24.dff", "start": 138042779, "end": 138044827}, {"filename": "/models/gta3.img/lodtaxifirm02.dff", "start": 138044827, "end": 138048923}, {"filename": "/models/gta3.img/lodtbuildws14.dff", "start": 138048923, "end": 138055067}, {"filename": "/models/gta3.img/lodtermaintfloor1.dff", "start": 138055067, "end": 138065307}, {"filename": "/models/gta3.img/lodtermina_01.dff", "start": 138065307, "end": 138077595}, {"filename": "/models/gta3.img/lodterminalb1.dff", "start": 138077595, "end": 138087835}, {"filename": "/models/gta3.img/lodti1rd.dff", "start": 138087835, "end": 138096027}, {"filename": "/models/gta3.img/lodtibuilds.dff", "start": 138096027, "end": 138104219}, {"filename": "/models/gta3.img/lodticent.dff", "start": 138104219, "end": 138108315}, {"filename": "/models/gta3.img/lodtides3_nt.dff", "start": 138108315, "end": 138116507}, {"filename": "/models/gta3.img/lodtihut1.dff", "start": 138116507, "end": 138136987}, {"filename": "/models/gta3.img/lodtihut2.dff", "start": 138136987, "end": 138155419}, {"filename": "/models/gta3.img/lodtihut3.dff", "start": 138155419, "end": 138167707}, {"filename": "/models/gta3.img/lodtimall.dff", "start": 138167707, "end": 138175899}, {"filename": "/models/gta3.img/lodtiroadbuild.dff", "start": 138175899, "end": 138192283}, {"filename": "/models/gta3.img/lodtiskyg.dff", "start": 138192283, "end": 138198427}, {"filename": "/models/gta3.img/lodtiskyt.dff", "start": 138198427, "end": 138204571}, {"filename": "/models/gta3.img/lodtistation.dff", "start": 138204571, "end": 138208667}, {"filename": "/models/gta3.img/lodtistation01.dff", "start": 138208667, "end": 138212763}, {"filename": "/models/gta3.img/lodtleha_hardware.dff", "start": 138212763, "end": 138216859}, {"filename": "/models/gta3.img/lodtleha_police.dff", "start": 138216859, "end": 138225051}, {"filename": "/models/gta3.img/lodtlehabroad.dff", "start": 138225051, "end": 138229147}, {"filename": "/models/gta3.img/lodtlehacoast02.dff", "start": 138229147, "end": 138235291}, {"filename": "/models/gta3.img/lodtlehacoast05.dff", "start": 138235291, "end": 138239387}, {"filename": "/models/gta3.img/lodtlharoadgg2.dff", "start": 138239387, "end": 138243483}, {"filename": "/models/gta3.img/lodtower.dff", "start": 138243483, "end": 138249627}, {"filename": "/models/gta3.img/lodtower_01.dff", "start": 138249627, "end": 138257819}, {"filename": "/models/gta3.img/lodtower_02.dff", "start": 138257819, "end": 138266011}, {"filename": "/models/gta3.img/lodtower_03.dff", "start": 138266011, "end": 138274203}, {"filename": "/models/gta3.img/lodtranspol1.dff", "start": 138274203, "end": 138278299}, {"filename": "/models/gta3.img/lodtroada.dff", "start": 138278299, "end": 138280347}, {"filename": "/models/gta3.img/lodtroadb.dff", "start": 138280347, "end": 138282395}, {"filename": "/models/gta3.img/lodtship_structure0.dff", "start": 138282395, "end": 138304923}, {"filename": "/models/gta3.img/loduild034.dff", "start": 138304923, "end": 138311067}, {"filename": "/models/gta3.img/loduild1b.dff", "start": 138311067, "end": 138315163}, {"filename": "/models/gta3.img/loduilding01.dff", "start": 138315163, "end": 138319259}, {"filename": "/models/gta3.img/loduilding05.dff", "start": 138319259, "end": 138327451}, {"filename": "/models/gta3.img/lodvroad1.dff", "start": 138327451, "end": 138329499}, {"filename": "/models/gta3.img/lodvroad2.dff", "start": 138329499, "end": 138331547}, {"filename": "/models/gta3.img/lodwalkway2.dff", "start": 138331547, "end": 138335643}, {"filename": "/models/gta3.img/lodwall.dff", "start": 138335643, "end": 138339739}, {"filename": "/models/gta3.img/lodwall2.dff", "start": 138339739, "end": 138341787}, {"filename": "/models/gta3.img/lodwall3.dff", "start": 138341787, "end": 138343835}, {"filename": "/models/gta3.img/lodwaretank.dff", "start": 138343835, "end": 138347931}, {"filename": "/models/gta3.img/lodwashnbrdge.txd", "start": 138347931, "end": 138354075}, {"filename": "/models/gta3.img/lodwashpaynspray.dff", "start": 138354075, "end": 138360219}, {"filename": "/models/gta3.img/lodwashsth.txd", "start": 138360219, "end": 138454427}, {"filename": "/models/gta3.img/lodwoodbridge1.dff", "start": 138454427, "end": 138458523}, {"filename": "/models/gta3.img/lodworksramps.dff", "start": 138458523, "end": 138464667}, {"filename": "/models/gta3.img/lodwshsth99.txd", "start": 138464667, "end": 138683803}, {"filename": "/models/gta3.img/lodwsouth08.txd", "start": 138683803, "end": 138923419}, {"filename": "/models/gta3.img/lodxrefhirise1.dff", "start": 138923419, "end": 138929563}, {"filename": "/models/gta3.img/lodxrefhot.txd", "start": 138929563, "end": 138962331}, {"filename": "/models/gta3.img/lodxrefhse1.dff", "start": 138962331, "end": 138966427}, {"filename": "/models/gta3.img/lodxrefhse2.dff", "start": 138966427, "end": 138970523}, {"filename": "/models/gta3.img/luggage.txd", "start": 138970523, "end": 139074971}, {"filename": "/models/gta3.img/m16.dff", "start": 139074971, "end": 139091355}, {"filename": "/models/gta3.img/m16.txd", "start": 139091355, "end": 139181467}, {"filename": "/models/gta3.img/male01.dff", "start": 139181467, "end": 139253147}, {"filename": "/models/gta3.img/male01.txd", "start": 139253147, "end": 139386267}, {"filename": "/models/gta3.img/mall.col", "start": 139386267, "end": 139589019}, {"filename": "/models/gta3.img/mallroof.txd", "start": 139589019, "end": 139591067}, {"filename": "/models/gta3.img/manana.dff", "start": 139591067, "end": 140147172}, {"filename": "/models/gta3.img/manana.txd", "start": 140147172, "end": 140999968}, {"filename": "/models/gta3.img/mansion.col", "start": 140999968, "end": 141153568}, {"filename": "/models/gta3.img/marquis.dff", "start": 141153568, "end": 141286688}, {"filename": "/models/gta3.img/marquis.txd", "start": 141286688, "end": 141321504}, {"filename": "/models/gta3.img/mbtbemp.dff", "start": 141321504, "end": 141341984}, {"filename": "/models/gta3.img/mbtbemp.txd", "start": 141341984, "end": 141489440}, {"filename": "/models/gta3.img/mc_ground3.dff", "start": 141489440, "end": 141499680}, {"filename": "/models/gta3.img/mc_ground4.dff", "start": 141499680, "end": 141509920}, {"filename": "/models/gta3.img/mc_ground5.dff", "start": 141509920, "end": 141518112}, {"filename": "/models/gta3.img/mc_overlights1.dff", "start": 141518112, "end": 141524256}, {"filename": "/models/gta3.img/mc_overlights2.dff", "start": 141524256, "end": 141536544}, {"filename": "/models/gta3.img/mc_wall2.dff", "start": 141536544, "end": 141546784}, {"filename": "/models/gta3.img/mc_wall3.dff", "start": 141546784, "end": 141559072}, {"filename": "/models/gta3.img/mcompounda1.txd", "start": 141559072, "end": 141974816}, {"filename": "/models/gta3.img/merced.dff", "start": 141974816, "end": 142253344}, {"filename": "/models/gta3.img/merced.txd", "start": 142253344, "end": 142337312}, {"filename": "/models/gta3.img/mesa.dff", "start": 142337312, "end": 142781649}, {"filename": "/models/gta3.img/mesa.txd", "start": 142781649, "end": 143632269}, {"filename": "/models/gta3.img/metal.txd", "start": 143632269, "end": 143652749}, {"filename": "/models/gta3.img/miamiland_kb02.dff", "start": 143652749, "end": 143673229}, {"filename": "/models/gta3.img/miamiland_kb03.dff", "start": 143673229, "end": 143677325}, {"filename": "/models/gta3.img/miamiland_kb04.dff", "start": 143677325, "end": 143693709}, {"filename": "/models/gta3.img/miamiland_kb11.dff", "start": 143693709, "end": 143712141}, {"filename": "/models/gta3.img/miamiland_kb11b.dff", "start": 143712141, "end": 143720333}, {"filename": "/models/gta3.img/miamiland_ws04.dff", "start": 143720333, "end": 143738765}, {"filename": "/models/gta3.img/miamiland_ws04b.dff", "start": 143738765, "end": 143744909}, {"filename": "/models/gta3.img/missile.dff", "start": 143744909, "end": 143751053}, {"filename": "/models/gta3.img/missile.txd", "start": 143751053, "end": 143755149}, {"filename": "/models/gta3.img/mitraffic.txd", "start": 143755149, "end": 143761293}, {"filename": "/models/gta3.img/mob_detailsb.dff", "start": 143761293, "end": 143857549}, {"filename": "/models/gta3.img/mob_door2.dff", "start": 143857549, "end": 143861645}, {"filename": "/models/gta3.img/mob_door3.dff", "start": 143861645, "end": 143865741}, {"filename": "/models/gta3.img/mob_mobroom2.dff", "start": 143865741, "end": 144025485}, {"filename": "/models/gta3.img/money.dff", "start": 144025485, "end": 144027533}, {"filename": "/models/gta3.img/moonbeam.dff", "start": 144027533, "end": 144534621}, {"filename": "/models/gta3.img/moonbeam.txd", "start": 144534621, "end": 145387417}, {"filename": "/models/gta3.img/mtraffic1.dff", "start": 145387417, "end": 145422233}, {"filename": "/models/gta3.img/mule.dff", "start": 145422233, "end": 145769035}, {"filename": "/models/gta3.img/mule.txd", "start": 145769035, "end": 146602147}, {"filename": "/models/gta3.img/nairport.txd", "start": 146602147, "end": 146901155}, {"filename": "/models/gta3.img/nbeach.col", "start": 146901155, "end": 147259555}, {"filename": "/models/gta3.img/nbeach_lod.txd", "start": 147259555, "end": 147378339}, {"filename": "/models/gta3.img/nbeachbt.col", "start": 147378339, "end": 147894435}, {"filename": "/models/gta3.img/nbeachw.col", "start": 147894435, "end": 148156579}, {"filename": "/models/gta3.img/nbw_grssypatch.txd", "start": 148156579, "end": 148160675}, {"filename": "/models/gta3.img/new_bushtest.dff", "start": 148160675, "end": 148164771}, {"filename": "/models/gta3.img/new_bushtest42.dff", "start": 148164771, "end": 148170915}, {"filename": "/models/gta3.img/newashbuild1.txd", "start": 148170915, "end": 148605091}, {"filename": "/models/gta3.img/nitestick.dff", "start": 148605091, "end": 148609187}, {"filename": "/models/gta3.img/nitestick.txd", "start": 148609187, "end": 148678819}, {"filename": "/models/gta3.img/noparkingsign1.dff", "start": 148678819, "end": 148682915}, {"filename": "/models/gta3.img/northbuild.txd", "start": 148682915, "end": 149452963}, {"filename": "/models/gta3.img/nrth1veg21.dff", "start": 149452963, "end": 149463203}, {"filename": "/models/gta3.img/nrth1veg42.dff", "start": 149463203, "end": 149495971}, {"filename": "/models/gta3.img/nrth3veg05.dff", "start": 149495971, "end": 149514403}, {"filename": "/models/gta3.img/nrth3veg08.dff", "start": 149514403, "end": 149532835}, {"filename": "/models/gta3.img/nrth3veg16.dff", "start": 149532835, "end": 149555363}, {"filename": "/models/gta3.img/nrth3veg25.dff", "start": 149555363, "end": 149590179}, {"filename": "/models/gta3.img/nrth3veg50.dff", "start": 149590179, "end": 149606563}, {"filename": "/models/gta3.img/nrth3veg59.dff", "start": 149606563, "end": 149629091}, {"filename": "/models/gta3.img/nrth4veg05.dff", "start": 149629091, "end": 149647523}, {"filename": "/models/gta3.img/nrth4veg21.dff", "start": 149647523, "end": 149659811}, {"filename": "/models/gta3.img/nrth4veg212.dff", "start": 149659811, "end": 149672099}, {"filename": "/models/gta3.img/nrth7veg.dff", "start": 149672099, "end": 149700771}, {"filename": "/models/gta3.img/nt_aircon1_01.dff", "start": 149700771, "end": 149706915}, {"filename": "/models/gta3.img/nt_aircon1dbl.dff", "start": 149706915, "end": 149717155}, {"filename": "/models/gta3.img/nt_aircon3_01.dff", "start": 149717155, "end": 149719203}, {"filename": "/models/gta3.img/nt_bed1_01.dff", "start": 149719203, "end": 149780643}, {"filename": "/models/gta3.img/nt_couch_1.dff", "start": 149780643, "end": 149794979}, {"filename": "/models/gta3.img/nt_wassily1_02.dff", "start": 149794979, "end": 149895331}, {"filename": "/models/gta3.img/oceandn.col", "start": 149895331, "end": 150186147}, {"filename": "/models/gta3.img/oceandrv.col", "start": 150186147, "end": 150364323}, {"filename": "/models/gta3.img/oceanic.dff", "start": 150364323, "end": 150935727}, {"filename": "/models/gta3.img/oceanic.txd", "start": 150935727, "end": 151794667}, {"filename": "/models/gta3.img/oceanrda03_dy.dff", "start": 151794667, "end": 151813099}, {"filename": "/models/gta3.img/oceanrda03_nt.dff", "start": 151813099, "end": 151833579}, {"filename": "/models/gta3.img/oceanrda04_dy.dff", "start": 151833579, "end": 151841771}, {"filename": "/models/gta3.img/oceanrda04_nt.dff", "start": 151841771, "end": 151852011}, {"filename": "/models/gta3.img/oceanrda05_nt.dff", "start": 151852011, "end": 151870443}, {"filename": "/models/gta3.img/oceanrda06_nt.dff", "start": 151870443, "end": 151882731}, {"filename": "/models/gta3.img/ocmiamistrip8.txd", "start": 151882731, "end": 152237035}, {"filename": "/models/gta3.img/ocmiamistrip9.txd", "start": 152237035, "end": 152964075}, {"filename": "/models/gta3.img/od_alleys3_01_dy.dff", "start": 152964075, "end": 152972267}, {"filename": "/models/gta3.img/od_alleys3_01_nt.dff", "start": 152972267, "end": 152980459}, {"filename": "/models/gta3.img/od_alleys3b_01_dy.dff", "start": 152980459, "end": 152990699}, {"filename": "/models/gta3.img/od_alleys3b_01_nt.dff", "start": 152990699, "end": 153000939}, {"filename": "/models/gta3.img/od_buildkb23_dy.dff", "start": 153000939, "end": 153013227}, {"filename": "/models/gta3.img/od_buildkb23_nt.dff", "start": 153013227, "end": 153025515}, {"filename": "/models/gta3.img/od_buildkb27_dy.dff", "start": 153025515, "end": 153070571}, {"filename": "/models/gta3.img/od_buildkb27_nt.dff", "start": 153070571, "end": 153115627}, {"filename": "/models/gta3.img/od_buildkb2_dy.dff", "start": 153115627, "end": 153158635}, {"filename": "/models/gta3.img/od_buildkb2_nt.dff", "start": 153158635, "end": 153199595}, {"filename": "/models/gta3.img/od_dirtshad4.dff", "start": 153199595, "end": 153211883}, {"filename": "/models/gta3.img/od_hotels1.txd", "start": 153211883, "end": 153629675}, {"filename": "/models/gta3.img/od_lightbeam.dff", "start": 153629675, "end": 153631723}, {"filename": "/models/gta3.img/od_majestic2c_dy.dff", "start": 153631723, "end": 153678827}, {"filename": "/models/gta3.img/od_majestic2c_nt.dff", "start": 153678827, "end": 153727979}, {"filename": "/models/gta3.img/od_neona1.dff", "start": 153727979, "end": 153730027}, {"filename": "/models/gta3.img/od_neonn.dff", "start": 153730027, "end": 153734123}, {"filename": "/models/gta3.img/od_neons1.txd", "start": 153734123, "end": 153805803}, {"filename": "/models/gta3.img/od_neons3b_nt.dff", "start": 153805803, "end": 153807851}, {"filename": "/models/gta3.img/od_neonx1.dff", "start": 153807851, "end": 153811947}, {"filename": "/models/gta3.img/odalleygb_dy.dff", "start": 153811947, "end": 153822187}, {"filename": "/models/gta3.img/odalleygb_nt.dff", "start": 153822187, "end": 153832427}, {"filename": "/models/gta3.img/odalleyground_dy.dff", "start": 153832427, "end": 153840619}, {"filename": "/models/gta3.img/odalleyground_nt.dff", "start": 153840619, "end": 153848811}, {"filename": "/models/gta3.img/oddoorway2.dff", "start": 153848811, "end": 153854955}, {"filename": "/models/gta3.img/odhighsandgrs1.dff", "start": 153854955, "end": 153875435}, {"filename": "/models/gta3.img/odlowsand1.dff", "start": 153875435, "end": 153877483}, {"filename": "/models/gta3.img/odlowsand2.dff", "start": 153877483, "end": 153879531}, {"filename": "/models/gta3.img/odneontest.dff", "start": 153879531, "end": 153881579}, {"filename": "/models/gta3.img/odneontest.txd", "start": 153881579, "end": 154022891}, {"filename": "/models/gta3.img/odnhotelspr.txd", "start": 154022891, "end": 154604523}, {"filename": "/models/gta3.img/odnroad04od_nt.dff", "start": 154604523, "end": 154625003}, {"filename": "/models/gta3.img/odnroad05od.dff", "start": 154625003, "end": 154633195}, {"filename": "/models/gta3.img/odnroad08_dy.dff", "start": 154633195, "end": 154643435}, {"filename": "/models/gta3.img/odnroad08_nt.dff", "start": 154643435, "end": 154655723}, {"filename": "/models/gta3.img/package1.dff", "start": 154655723, "end": 154661867}, {"filename": "/models/gta3.img/package1.txd", "start": 154661867, "end": 154665963}, {"filename": "/models/gta3.img/packer.dff", "start": 154665963, "end": 155202269}, {"filename": "/models/gta3.img/packer.txd", "start": 155202269, "end": 155973941}, {"filename": "/models/gta3.img/pcj600.dff", "start": 155973941, "end": 156094773}, {"filename": "/models/gta3.img/pcj600.txd", "start": 156094773, "end": 156500277}, {"filename": "/models/gta3.img/peren.dff", "start": 156500277, "end": 156948644}, {"filename": "/models/gta3.img/peren.txd", "start": 156948644, "end": 157801440}, {"filename": "/models/gta3.img/pheonix.dff", "start": 157801440, "end": 158522035}, {"filename": "/models/gta3.img/pheonix.txd", "start": 158522035, "end": 159265323}, {"filename": "/models/gta3.img/plants01.dff", "start": 159265323, "end": 159279659}, {"filename": "/models/gta3.img/player.dff", "start": 159279659, "end": 159367723}, {"filename": "/models/gta3.img/player.txd", "start": 159367723, "end": 159500843}, {"filename": "/models/gta3.img/playidles.ifp", "start": 159500843, "end": 159586859}, {"filename": "/models/gta3.img/police.dff", "start": 159586859, "end": 160281144}, {"filename": "/models/gta3.img/police.txd", "start": 160281144, "end": 161407352}, {"filename": "/models/gta3.img/pony.dff", "start": 161407352, "end": 161909608}, {"filename": "/models/gta3.img/pony.txd", "start": 161909608, "end": 162674880}, {"filename": "/models/gta3.img/property_fsale.dff", "start": 162674880, "end": 162681024}, {"filename": "/models/gta3.img/property_locked.dff", "start": 162681024, "end": 162687168}, {"filename": "/models/gta3.img/radar32.txd", "start": 162687168, "end": 162754752}, {"filename": "/models/gta3.img/radar33.txd", "start": 162754752, "end": 162822336}, {"filename": "/models/gta3.img/radar35.txd", "start": 162822336, "end": 162889920}, {"filename": "/models/gta3.img/radar36.txd", "start": 162889920, "end": 162957504}, {"filename": "/models/gta3.img/radar37.txd", "start": 162957504, "end": 163025088}, {"filename": "/models/gta3.img/radar40.txd", "start": 163025088, "end": 163092672}, {"filename": "/models/gta3.img/radar41.txd", "start": 163092672, "end": 163160256}, {"filename": "/models/gta3.img/radar43.txd", "start": 163160256, "end": 163227840}, {"filename": "/models/gta3.img/radar44.txd", "start": 163227840, "end": 163295424}, {"filename": "/models/gta3.img/radar45.txd", "start": 163295424, "end": 163363008}, {"filename": "/models/gta3.img/radar48.txd", "start": 163363008, "end": 163430592}, {"filename": "/models/gta3.img/radar49.txd", "start": 163430592, "end": 163498176}, {"filename": "/models/gta3.img/radar50.txd", "start": 163498176, "end": 163565760}, {"filename": "/models/gta3.img/radar51.txd", "start": 163565760, "end": 163633344}, {"filename": "/models/gta3.img/radar52.txd", "start": 163633344, "end": 163700928}, {"filename": "/models/gta3.img/radar53.txd", "start": 163700928, "end": 163768512}, {"filename": "/models/gta3.img/radar56.txd", "start": 163768512, "end": 163836096}, {"filename": "/models/gta3.img/radar57.txd", "start": 163836096, "end": 163903680}, {"filename": "/models/gta3.img/radar58.txd", "start": 163903680, "end": 163971264}, {"filename": "/models/gta3.img/radar59.txd", "start": 163971264, "end": 164038848}, {"filename": "/models/gta3.img/rancher.dff", "start": 164038848, "end": 164631074}, {"filename": "/models/gta3.img/rancher.txd", "start": 164631074, "end": 165024690}, {"filename": "/models/gta3.img/rcgrenade.dff", "start": 165024690, "end": 165030834}, {"filename": "/models/gta3.img/rcgrenade.txd", "start": 165030834, "end": 165100466}, {"filename": "/models/gta3.img/reefer.dff", "start": 165100466, "end": 165211058}, {"filename": "/models/gta3.img/reefer.txd", "start": 165211058, "end": 165262258}, {"filename": "/models/gta3.img/regina.dff", "start": 165262258, "end": 165888785}, {"filename": "/models/gta3.img/regina.txd", "start": 165888785, "end": 166741581}, {"filename": "/models/gta3.img/rifle.ifp", "start": 166741581, "end": 166794829}, {"filename": "/models/gta3.img/rio.dff", "start": 166794829, "end": 166923853}, {"filename": "/models/gta3.img/rio.txd", "start": 166923853, "end": 166960717}, {"filename": "/models/gta3.img/rockpatch03.dff", "start": 166960717, "end": 166973005}, {"filename": "/models/gta3.img/rocky.txd", "start": 166973005, "end": 166985293}, {"filename": "/models/gta3.img/rumpo.dff", "start": 166985293, "end": 167491398}, {"filename": "/models/gta3.img/rumpo.txd", "start": 167491398, "end": 168273182}, {"filename": "/models/gta3.img/rustship_structure0.dff", "start": 168273182, "end": 168387870}, {"filename": "/models/gta3.img/sabre.dff", "start": 168387870, "end": 169106211}, {"filename": "/models/gta3.img/sabre.txd", "start": 169106211, "end": 170204903}, {"filename": "/models/gta3.img/sanchez.dff", "start": 170204903, "end": 170299111}, {"filename": "/models/gta3.img/sanchez.txd", "start": 170299111, "end": 170329831}, {"filename": "/models/gta3.img/schair.dff", "start": 170329831, "end": 170340071}, {"filename": "/models/gta3.img/schair.txd", "start": 170340071, "end": 170352359}, {"filename": "/models/gta3.img/seabed.txd", "start": 170352359, "end": 170419943}, {"filename": "/models/gta3.img/securica.dff", "start": 170419943, "end": 170902560}, {"filename": "/models/gta3.img/securica.txd", "start": 170902560, "end": 171684344}, {"filename": "/models/gta3.img/sentinel.dff", "start": 171684344, "end": 172581893}, {"filename": "/models/gta3.img/sentinel.txd", "start": 172581893, "end": 173325181}, {"filename": "/models/gta3.img/sentxs.dff", "start": 173325181, "end": 174096898}, {"filename": "/models/gta3.img/sentxs.txd", "start": 174096898, "end": 174840186}, {"filename": "/models/gta3.img/sfrenda.dff", "start": 174840186, "end": 174920058}, {"filename": "/models/gta3.img/sfrenda.txd", "start": 174920058, "end": 175051130}, {"filename": "/models/gta3.img/sfrendb.dff", "start": 175051130, "end": 175133050}, {"filename": "/models/gta3.img/sfrendb.txd", "start": 175133050, "end": 175309178}, {"filename": "/models/gta3.img/sgoona.dff", "start": 175309178, "end": 175382906}, {"filename": "/models/gta3.img/sgoona.txd", "start": 175382906, "end": 175427962}, {"filename": "/models/gta3.img/sgoonb.dff", "start": 175427962, "end": 175503738}, {"filename": "/models/gta3.img/sgoonb.txd", "start": 175503738, "end": 175548794}, {"filename": "/models/gta3.img/shared_beach.txd", "start": 175548794, "end": 175923578}, {"filename": "/models/gta3.img/sharedalleyod.txd", "start": 175923578, "end": 176261498}, {"filename": "/models/gta3.img/shark.dff", "start": 176261498, "end": 176273786}, {"filename": "/models/gta3.img/shark.txd", "start": 176273786, "end": 176279930}, {"filename": "/models/gta3.img/shotgun.ifp", "start": 176279930, "end": 176320890}, {"filename": "/models/gta3.img/shotgun2.dff", "start": 176320890, "end": 176341370}, {"filename": "/models/gta3.img/shotgun2.txd", "start": 176341370, "end": 176411002}, {"filename": "/models/gta3.img/signs.txd", "start": 176411002, "end": 176454010}, {"filename": "/models/gta3.img/speeder.txd", "start": 176454010, "end": 176492922}, {"filename": "/models/gta3.img/stadint.col", "start": 176492922, "end": 176927098}, {"filename": "/models/gta3.img/stallion.dff", "start": 176927098, "end": 177547041}, {"filename": "/models/gta3.img/stallion.txd", "start": 177547041, "end": 178290329}, {"filename": "/models/gta3.img/starisl.col", "start": 178290329, "end": 178538137}, {"filename": "/models/gta3.img/steyr.dff", "start": 178538137, "end": 178554521}, {"filename": "/models/gta3.img/steyr.ifp", "start": 178554521, "end": 178591385}, {"filename": "/models/gta3.img/steyr.txd", "start": 178591385, "end": 178665113}, {"filename": "/models/gta3.img/stiltsville03.dff", "start": 178665113, "end": 178761369}, {"filename": "/models/gta3.img/stiltsville05.dff", "start": 178761369, "end": 178925209}, {"filename": "/models/gta3.img/stiltsville1.dff", "start": 178925209, "end": 179089049}, {"filename": "/models/gta3.img/stilty.txd", "start": 179089049, "end": 179216025}, {"filename": "/models/gta3.img/stinger.dff", "start": 179216025, "end": 179933412}, {"filename": "/models/gta3.img/stinger.txd", "start": 179933412, "end": 181027272}, {"filename": "/models/gta3.img/streetlamp1.dff", "start": 181027272, "end": 181033416}, {"filename": "/models/gta3.img/streetlamp2.dff", "start": 181033416, "end": 181039560}, {"filename": "/models/gta3.img/stripback.txd", "start": 181039560, "end": 181354952}, {"filename": "/models/gta3.img/stripclb.col", "start": 181354952, "end": 181385672}, {"filename": "/models/gta3.img/stripclbdrclsd.dff", "start": 181385672, "end": 181387720}, {"filename": "/models/gta3.img/strpbckdrclsd.dff", "start": 181387720, "end": 181389768}, {"filename": "/models/gta3.img/sub_floodlite.dff", "start": 181389768, "end": 181397960}, {"filename": "/models/gta3.img/sunbathe.ifp", "start": 181397960, "end": 181457352}, {"filename": "/models/gta3.img/svntray.dff", "start": 181457352, "end": 181463496}, {"filename": "/models/gta3.img/svntray.txd", "start": 181463496, "end": 181475784}, {"filename": "/models/gta3.img/swashlandb.txd", "start": 181475784, "end": 181950920}, {"filename": "/models/gta3.img/taxi.dff", "start": 181950920, "end": 182626769}, {"filename": "/models/gta3.img/taxi.txd", "start": 182626769, "end": 184010501}, {"filename": "/models/gta3.img/telegraph.txd", "start": 184010501, "end": 184065797}, {"filename": "/models/gta3.img/telgrphpole02.dff", "start": 184065797, "end": 184071941}, {"filename": "/models/gta3.img/trash.dff", "start": 184071941, "end": 184527027}, {"filename": "/models/gta3.img/trash.txd", "start": 184527027, "end": 185292299}, {"filename": "/models/gta3.img/treeshad.txd", "start": 185292299, "end": 185327115}, {"filename": "/models/gta3.img/treeshadairha05.dff", "start": 185327115, "end": 185339403}, {"filename": "/models/gta3.img/treeshadairha06.dff", "start": 185339403, "end": 185345547}, {"filename": "/models/gta3.img/treeshadairha07.dff", "start": 185345547, "end": 185351691}, {"filename": "/models/gta3.img/treeshadwn01.dff", "start": 185351691, "end": 185374219}, {"filename": "/models/gta3.img/treeshadwn02.dff", "start": 185374219, "end": 185382411}, {"filename": "/models/gta3.img/treeshadwn03.dff", "start": 185382411, "end": 185388555}, {"filename": "/models/gta3.img/turtle.dff", "start": 185388555, "end": 185409035}, {"filename": "/models/gta3.img/turtle.txd", "start": 185409035, "end": 185419275}, {"filename": "/models/gta3.img/uzi.dff", "start": 185419275, "end": 185431563}, {"filename": "/models/gta3.img/uzi.ifp", "start": 185431563, "end": 185480715}, {"filename": "/models/gta3.img/uzi.txd", "start": 185480715, "end": 185554443}, {"filename": "/models/gta3.img/uziold.dff", "start": 185554443, "end": 185572875}, {"filename": "/models/gta3.img/uziold.txd", "start": 185572875, "end": 185642507}, {"filename": "/models/gta3.img/van.ifp", "start": 185642507, "end": 185722379}, {"filename": "/models/gta3.img/veg_palm01.dff", "start": 185722379, "end": 185726475}, {"filename": "/models/gta3.img/veg_palm02.dff", "start": 185726475, "end": 185730571}, {"filename": "/models/gta3.img/veg_palm04.dff", "start": 185730571, "end": 185734667}, {"filename": "/models/gta3.img/veg_palmbig14.dff", "start": 185734667, "end": 185740811}, {"filename": "/models/gta3.img/veg_palmkbb11.dff", "start": 185740811, "end": 185744907}, {"filename": "/models/gta3.img/veg_palwee01.dff", "start": 185744907, "end": 185749003}, {"filename": "/models/gta3.img/vice1.dff", "start": 185749003, "end": 185822731}, {"filename": "/models/gta3.img/vice1.txd", "start": 185822731, "end": 185867787}, {"filename": "/models/gta3.img/vice2.dff", "start": 185867787, "end": 185945611}, {"filename": "/models/gta3.img/vice2.txd", "start": 185945611, "end": 185990667}, {"filename": "/models/gta3.img/vice3.dff", "start": 185990667, "end": 186068491}, {"filename": "/models/gta3.img/vice3.txd", "start": 186068491, "end": 186113547}, {"filename": "/models/gta3.img/vice4.dff", "start": 186113547, "end": 186187275}, {"filename": "/models/gta3.img/vice4.txd", "start": 186187275, "end": 186232331}, {"filename": "/models/gta3.img/vice5.dff", "start": 186232331, "end": 186308107}, {"filename": "/models/gta3.img/vice5.txd", "start": 186308107, "end": 186375691}, {"filename": "/models/gta3.img/vice6.dff", "start": 186375691, "end": 186453515}, {"filename": "/models/gta3.img/vice6.txd", "start": 186453515, "end": 186521099}, {"filename": "/models/gta3.img/vice7.dff", "start": 186521099, "end": 186594827}, {"filename": "/models/gta3.img/vice7.txd", "start": 186594827, "end": 186639883}, {"filename": "/models/gta3.img/vice8.dff", "start": 186639883, "end": 186717707}, {"filename": "/models/gta3.img/vice8.txd", "start": 186717707, "end": 186762763}, {"filename": "/models/gta3.img/virgo.dff", "start": 186762763, "end": 187254270}, {"filename": "/models/gta3.img/virgo.txd", "start": 187254270, "end": 188109242}, {"filename": "/models/gta3.img/walton.dff", "start": 188109242, "end": 188604660}, {"filename": "/models/gta3.img/walton.txd", "start": 188604660, "end": 189369932}, {"filename": "/models/gta3.img/wash_deco01.dff", "start": 189369932, "end": 189457996}, {"filename": "/models/gta3.img/wash_deco012.dff", "start": 189457996, "end": 189486668}, {"filename": "/models/gta3.img/wash_deco02.dff", "start": 189486668, "end": 189541964}, {"filename": "/models/gta3.img/wash_deco022.dff", "start": 189541964, "end": 189554252}, {"filename": "/models/gta3.img/wash_deco03.dff", "start": 189554252, "end": 189611596}, {"filename": "/models/gta3.img/wash_deco032.dff", "start": 189611596, "end": 189617740}, {"filename": "/models/gta3.img/wash_pizzaplace.dff", "start": 189617740, "end": 189634124}, {"filename": "/models/gta3.img/washawning2b.dff", "start": 189634124, "end": 189648460}, {"filename": "/models/gta3.img/washawninga1.dff", "start": 189648460, "end": 189662796}, {"filename": "/models/gta3.img/washbballcrt.txd", "start": 189662796, "end": 189818444}, {"filename": "/models/gta3.img/washbuild003.dff", "start": 189818444, "end": 189834828}, {"filename": "/models/gta3.img/washbuild005.dff", "start": 189834828, "end": 189855308}, {"filename": "/models/gta3.img/washbuild014.dff", "start": 189855308, "end": 189873740}, {"filename": "/models/gta3.img/washbuild068.dff", "start": 189873740, "end": 189926988}, {"filename": "/models/gta3.img/washbuild071.dff", "start": 189926988, "end": 189937228}, {"filename": "/models/gta3.img/washbuild072.dff", "start": 189937228, "end": 189949516}, {"filename": "/models/gta3.img/washbuild073.dff", "start": 189949516, "end": 189990476}, {"filename": "/models/gta3.img/washbuild074.dff", "start": 189990476, "end": 190000716}, {"filename": "/models/gta3.img/washbuild075.dff", "start": 190000716, "end": 190010956}, {"filename": "/models/gta3.img/washbuild116.dff", "start": 190010956, "end": 190015052}, {"filename": "/models/gta3.img/washbuild120.txd", "start": 190015052, "end": 190529100}, {"filename": "/models/gta3.img/washbuild124.txd", "start": 190529100, "end": 190666316}, {"filename": "/models/gta3.img/washbuild187.dff", "start": 190666316, "end": 190719564}, {"filename": "/models/gta3.img/washbuild198.dff", "start": 190719564, "end": 190787148}, {"filename": "/models/gta3.img/washcorn2.txd", "start": 190787148, "end": 191073868}, {"filename": "/models/gta3.img/washing.dff", "start": 191073868, "end": 191631618}, {"filename": "/models/gta3.img/washing.txd", "start": 191631618, "end": 192396890}, {"filename": "/models/gta3.img/washintn.col", "start": 192396890, "end": 192673370}, {"filename": "/models/gta3.img/washints.col", "start": 192673370, "end": 192990810}, {"filename": "/models/gta3.img/washlandb.txd", "start": 192990810, "end": 193138266}, {"filename": "/models/gta3.img/washmallnew.txd", "start": 193138266, "end": 193922650}, {"filename": "/models/gta3.img/washmallpark.txd", "start": 193922650, "end": 194119258}, {"filename": "/models/gta3.img/washmiamistrip2.txd", "start": 194119258, "end": 194688602}, {"filename": "/models/gta3.img/washmiamistrip3.txd", "start": 194688602, "end": 195167834}, {"filename": "/models/gta3.img/washmiamistrip4.txd", "start": 195167834, "end": 195718746}, {"filename": "/models/gta3.img/washnewbuild1.txd", "start": 195718746, "end": 195968602}, {"filename": "/models/gta3.img/washnewbuild2.txd", "start": 195968602, "end": 196322906}, {"filename": "/models/gta3.img/washnewbuild21.txd", "start": 196322906, "end": 196636250}, {"filename": "/models/gta3.img/washnewsky1.dff", "start": 196636250, "end": 196718170}, {"filename": "/models/gta3.img/washnewsky2.dff", "start": 196718170, "end": 196742746}, {"filename": "/models/gta3.img/washnewsky3.dff", "start": 196742746, "end": 196824666}, {"filename": "/models/gta3.img/washnrthpstr1.txd", "start": 196824666, "end": 196871770}, {"filename": "/models/gta3.img/washotelneon1.dff", "start": 196871770, "end": 196922970}, {"filename": "/models/gta3.img/washpshoutdet.dff", "start": 196922970, "end": 196992602}, {"filename": "/models/gta3.img/washpshoutveg.dff", "start": 196992602, "end": 197043802}, {"filename": "/models/gta3.img/washsky1.txd", "start": 197043802, "end": 197412442}, {"filename": "/models/gta3.img/washskyplant1.dff", "start": 197412442, "end": 197473882}, {"filename": "/models/gta3.img/washskyplant2.dff", "start": 197473882, "end": 197535322}, {"filename": "/models/gta3.img/washvegy241.dff", "start": 197535322, "end": 197566042}, {"filename": "/models/gta3.img/washwater.txd", "start": 197566042, "end": 197633626}, {"filename": "/models/gta3.img/wfogo.dff", "start": 197633626, "end": 197697114}, {"filename": "/models/gta3.img/wfogo.txd", "start": 197697114, "end": 197764698}, {"filename": "/models/gta3.img/wfost.dff", "start": 197764698, "end": 197832282}, {"filename": "/models/gta3.img/wfost.txd", "start": 197832282, "end": 197899866}, {"filename": "/models/gta3.img/wfotr.dff", "start": 197899866, "end": 197965402}, {"filename": "/models/gta3.img/wfotr.txd", "start": 197965402, "end": 198010458}, {"filename": "/models/gta3.img/wfybu.dff", "start": 198010458, "end": 198075994}, {"filename": "/models/gta3.img/wfybu.txd", "start": 198075994, "end": 198121050}, {"filename": "/models/gta3.img/wfypr.dff", "start": 198121050, "end": 198182490}, {"filename": "/models/gta3.img/wfypr.txd", "start": 198182490, "end": 198227546}, {"filename": "/models/gta3.img/wfyri.dff", "start": 198227546, "end": 198291034}, {"filename": "/models/gta3.img/wfyri.txd", "start": 198291034, "end": 198379098}, {"filename": "/models/gta3.img/wfysh.dff", "start": 198379098, "end": 198442586}, {"filename": "/models/gta3.img/wfysh.txd", "start": 198442586, "end": 198618714}, {"filename": "/models/gta3.img/wmobu.dff", "start": 198618714, "end": 198684250}, {"filename": "/models/gta3.img/wmobu.txd", "start": 198684250, "end": 198817370}, {"filename": "/models/gta3.img/wmori.dff", "start": 198817370, "end": 198882906}, {"filename": "/models/gta3.img/wmori.txd", "start": 198882906, "end": 199124570}, {"filename": "/models/gta3.img/wmost.dff", "start": 199124570, "end": 199204442}, {"filename": "/models/gta3.img/wmost.txd", "start": 199204442, "end": 199272026}, {"filename": "/models/gta3.img/wmotr.dff", "start": 199272026, "end": 199327322}, {"filename": "/models/gta3.img/wmotr.txd", "start": 199327322, "end": 199480922}, {"filename": "/models/gta3.img/wmybu.dff", "start": 199480922, "end": 199552602}, {"filename": "/models/gta3.img/wmybu.txd", "start": 199552602, "end": 199620186}, {"filename": "/models/gta3.img/wmycr.dff", "start": 199620186, "end": 199693914}, {"filename": "/models/gta3.img/wmycr.txd", "start": 199693914, "end": 199761498}, {"filename": "/models/gta3.img/wmypi.dff", "start": 199761498, "end": 199841370}, {"filename": "/models/gta3.img/wmypi.txd", "start": 199841370, "end": 200017498}, {"filename": "/models/gta3.img/wmyri.dff", "start": 200017498, "end": 200097370}, {"filename": "/models/gta3.img/wmyri.txd", "start": 200097370, "end": 200273498}, {"filename": "/models/gta3.img/wsh_roadswsh05.dff", "start": 200273498, "end": 200279642}, {"filename": "/models/gta3.img/wshbldws26bit.dff", "start": 200279642, "end": 200296026}, {"filename": "/models/gta3.img/wshbuil19wall.dff", "start": 200296026, "end": 200306266}, {"filename": "/models/gta3.img/wshbuildws09.dff", "start": 200306266, "end": 200437338}, {"filename": "/models/gta3.img/wshbuildws19.dff", "start": 200437338, "end": 200502874}, {"filename": "/models/gta3.img/wshbuildws26.dff", "start": 200502874, "end": 200517210}, {"filename": "/models/gta3.img/wshbuildws31.dff", "start": 200517210, "end": 200533594}, {"filename": "/models/gta3.img/wshbuildws312.dff", "start": 200533594, "end": 200547930}, {"filename": "/models/gta3.img/wshbuildws322.dff", "start": 200547930, "end": 200562266}, {"filename": "/models/gta3.img/wshbuildws43.dff", "start": 200562266, "end": 200644186}, {"filename": "/models/gta3.img/wshmalsign4.dff", "start": 200644186, "end": 200652378}, {"filename": "/models/gta3.img/wshmoundlaw1.dff", "start": 200652378, "end": 200658522}, {"filename": "/models/gta3.img/wshneon.txd", "start": 200658522, "end": 200664666}, {"filename": "/models/gta3.img/wshnrthpstr07.dff", "start": 200664666, "end": 200666714}, {"filename": "/models/gta3.img/wshnrthroad01.dff", "start": 200666714, "end": 200670810}, {"filename": "/models/gta3.img/wshnrthroad02.dff", "start": 200670810, "end": 200676954}, {"filename": "/models/gta3.img/wshnrthroad10.dff", "start": 200676954, "end": 200683098}, {"filename": "/models/gta3.img/wshnrthroad11.dff", "start": 200683098, "end": 200691290}, {"filename": "/models/gta3.img/wshnrthroad12.dff", "start": 200691290, "end": 200695386}, {"filename": "/models/gta3.img/wshnrthroad13.dff", "start": 200695386, "end": 200701530}, {"filename": "/models/gta3.img/wshnrthroad14.dff", "start": 200701530, "end": 200707674}, {"filename": "/models/gta3.img/wshnrthroad18.dff", "start": 200707674, "end": 200711770}, {"filename": "/models/gta3.img/wshnrthroad19.dff", "start": 200711770, "end": 200715866}, {"filename": "/models/gta3.img/wshnrthroad21.dff", "start": 200715866, "end": 200722010}, {"filename": "/models/gta3.img/wshnrthroad22.dff", "start": 200722010, "end": 200726106}, {"filename": "/models/gta3.img/wshnrthroad24.dff", "start": 200726106, "end": 200730202}, {"filename": "/models/gta3.img/wshnrthroad26.dff", "start": 200730202, "end": 200736346}, {"filename": "/models/gta3.img/wshnrthroad27.dff", "start": 200736346, "end": 200740442}, {"filename": "/models/gta3.img/wshscarland226.dff", "start": 200740442, "end": 200767066}, {"filename": "/models/gta3.img/wshsthroad05.dff", "start": 200767066, "end": 200771162}, {"filename": "/models/gta3.img/wshsthroad16.dff", "start": 200771162, "end": 200779354}, {"filename": "/models/gta3.img/wshtelgrph.txd", "start": 200779354, "end": 200799834}, {"filename": "/models/gta3.img/wshtelgrphcabl03.dff", "start": 200799834, "end": 200808026}, {"filename": "/models/gta3.img/wshtelgrphcabl04.dff", "start": 200808026, "end": 200816218}, {"filename": "/models/gta3.img/wshtelgrphcabl05.dff", "start": 200816218, "end": 200824410}, {"filename": "/models/gta3.img/wshxreflod.txd", "start": 200824410, "end": 200844890}, {"filename": "/models/gta3.img/xod_majestic_dy.dff", "start": 200844890, "end": 200900186}, {"filename": "/models/gta3.img/xod_majestic_nt.dff", "start": 200900186, "end": 200947290}, {"filename": "/models/gta3.img/xod_starlite_dy.dff", "start": 200947290, "end": 200990298}, {"filename": "/models/gta3.img/xod_starlite_nt.dff", "start": 200990298, "end": 201025114}, {"filename": "/models/gta3.img/xrefhotels.txd", "start": 201025114, "end": 201268826}, {"filename": "/models/gta3.img/yacht.col", "start": 201268826, "end": 201295450}, {"filename": "/models/gta3.img/yacht_lod.txd", "start": 201295450, "end": 201301594}, {"filename": "/models/gta3.img/yankee.dff", "start": 201301594, "end": 201714388}, {"filename": "/models/gta3.img/yankee.txd", "start": 201714388, "end": 202181988}, {"filename": "/models/hud.txd", "start": 202181988, "end": 208514956}, {"filename": "/models/indicator_caps.json", "start": 208514956, "end": 208540442}, {"filename": "/models/nswbtns.txd", "start": 208540442, "end": 209068866}, {"filename": "/models/particle.txd", "start": 209068866, "end": 211736938}, {"filename": "/models/ps3btns.txd", "start": 211736938, "end": 212265362}, {"filename": "/models/ps4btns.txd", "start": 212265362, "end": 212793786}, {"filename": "/models/ps5btns.txd", "start": 212793786, "end": 213322210}, {"filename": "/models/touchbtns.txd", "start": 213322210, "end": 213767738}, {"filename": "/models/x1btns.txd", "start": 213767738, "end": 214296162}, {"filename": "/models/x360btns.txd", "start": 214296162, "end": 214824586}, {"filename": "/neo/carTweakingTable.dat", "start": 214824586, "end": 214832077}, {"filename": "/neo/neo.txd", "start": 214832077, "end": 214861045}, {"filename": "/neo/rimTweakingTable.dat", "start": 214861045, "end": 214870049}, {"filename": "/neo/worldTweakingTable.dat", "start": 214870049, "end": 214871292}, {"filename": "/skins/texture_guide.jpg", "start": 214871292, "end": 214959431}, {"filename": "/txd/INTRO3.TXD", "start": 214959431, "end": 215222767}, {"filename": "/txd/LOADSC0.TXD", "start": 215222767, "end": 215486103}, {"filename": "/txd/LOADSC1.TXD", "start": 215486103, "end": 215749439}, {"filename": "/txd/LOADSC10.TXD", "start": 215749439, "end": 216012775}, {"filename": "/txd/LOADSC11.TXD", "start": 216012775, "end": 216276111}, {"filename": "/txd/LOADSC12.TXD", "start": 216276111, "end": 216539447}, {"filename": "/txd/LOADSC13.TXD", "start": 216539447, "end": 216802783}, {"filename": "/txd/LOADSC2.TXD", "start": 216802783, "end": 217066119}, {"filename": "/txd/LOADSC3.TXD", "start": 217066119, "end": 217329455}, {"filename": "/txd/LOADSC4.TXD", "start": 217329455, "end": 217592791}, {"filename": "/txd/LOADSC5.TXD", "start": 217592791, "end": 217856127}, {"filename": "/txd/LOADSC6.TXD", "start": 217856127, "end": 218119463}, {"filename": "/txd/LOADSC7.TXD", "start": 218119463, "end": 218382799}, {"filename": "/txd/LOADSC8.TXD", "start": 218382799, "end": 218646135}, {"filename": "/txd/LOADSC9.TXD", "start": 218646135, "end": 218909471}, {"filename": "/txd/NEWS.TXD", "start": 218909471, "end": 219172807}, {"filename": "/txd/SPLASH1.TXD", "start": 219172807, "end": 219305071}, {"filename": "/txd/SPLASH2.TXD", "start": 219305071, "end": 219437335}, {"filename": "/txd/SPLASH3.TXD", "start": 219437335, "end": 219437375}, {"filename": "/txd/intro1.txd", "start": 219437375, "end": 219700711}, {"filename": "/txd/intro2.txd", "start": 219700711, "end": 219964047}, {"filename": "/txd/intro4.txd", "start": 219964047, "end": 220227383}, {"filename": "/txd/newspapers.txd", "start": 220227383, "end": 221151463}, {"filename": "/txd/outro.txd", "start": 221151463, "end": 221414799}], "remote_package_size": 221414799});

  })();

// end include: C:\Users\s0rno\AppData\Local\Temp\tmp_hbod3e9.js
// include: C:/Users/s0rno/OneDrive/Documents/lance-vance/gta_vc_browser/web/ondemand.js
// Capa on-demand (pre-js): ficheros sueltos bajo /streamed/ + caché IDB.
//
// El motor pide ficheros por su ruta (p. ej. "MODELS\GTA3.IMG" o
// "models/gta3.img/cop.dff"). OD.ensure() los resuelve contra el
// manifiesto, los trae (IDB primero, red después), los escribe en MEMFS
// y devuelve la ruta canónica. Se invoca desde C vía EM_ASYNC_JS, así el
// hilo principal nunca se bloquea: la página sigue viva durante la descarga.
// Fuente del worker de carga (Nivel 1). Va embebida aquí y se crea con un
// Blob: así el port no necesita servir un fichero extra ni saber dónde está.
function odLog(s) {
  try { (globalThis.__odq = globalThis.__odq || []).push(String(s)); } catch (e) {}
}

var WORKER_SRC = `
var DB = null, BASE = '/streamed/', INFLIGHT = {};
function openDb() {
  return new Promise(function (res) {
    try {
      var q = indexedDB.open('vcod2', 2);
      q.onupgradeneeded = function (e) {
        var db = e.target.result;
        if (!db.objectStoreNames.contains('files')) db.createObjectStore('files');
        if (!db.objectStoreNames.contains('meta')) db.createObjectStore('meta');
      };
      q.onsuccess = function (e) { res(e.target.result); };
      q.onerror = function () { res(null); };
    } catch (e) { res(null); }
  });
}
function idbGet(k) {
  return new Promise(function (res) {
    if (!DB) return res(null);
    try {
      var q = DB.transaction(['files'], 'readonly').objectStore('files').get(k);
      q.onsuccess = function (e) { res(e.target.result || null); };
      q.onerror = function () { res(null); };
    } catch (e) { res(null); }
  });
}
function idbPut(k, buf) {
  return new Promise(function (res) {
    if (!DB) return res();
    try {
      var t = DB.transaction(['files', 'meta'], 'readwrite');
      t.objectStore('files').put(buf, k);
      t.objectStore('meta').put({ t: Date.now(), s: buf.byteLength }, k);
      t.oncomplete = function () { res(); };
      t.onerror = function () { res(); };
    } catch (e) { res(); }
  });
}
function isHtml(buf) {
  try {
    var u = new Uint8Array(buf, 0, Math.min(9, buf.byteLength));
    var s = String.fromCharCode.apply(null, u).toLowerCase();
    return s.indexOf('<!doctype') === 0 || s.indexOf('<html') === 0;
  } catch (e) { return false; }
}
function fromNet(key, rel, size) {
  return new Promise(function (res, rej) {
    var ctrl = new AbortController();
    var timer = setTimeout(function () { try { ctrl.abort(); } catch (e) {} }, 25000);
    var url = BASE + rel.split('/').map(encodeURIComponent).join('/');
    fetch(url, { signal: ctrl.signal }).then(function (r) {
      if (!r.ok) throw new Error('HTTP ' + r.status);
      var ct = (r.headers.get('content-type') || '').toLowerCase();
      if (ct.indexOf('text/html') >= 0) throw new Error('HTML en vez de fichero');
      return r.arrayBuffer();
    }).then(function (buf) {
      clearTimeout(timer);
      if (!buf || !buf.byteLength) throw new Error('fichero vacio');
      if (isHtml(buf)) throw new Error('HTML en vez de fichero');
      if (size && buf.byteLength < size) throw new Error('truncado ' + buf.byteLength + '/' + size);
      idbPut(key, buf);
      res({ buf: buf, src: 'net' });
    }).catch(function (e) { clearTimeout(timer); rej(e); });
  });
}
onmessage = function (ev) {
  var m = ev.data || {};
  if (m.op === 'init') {
    if (m.streamedUrl) BASE = m.streamedUrl;
    openDb().then(function (db) { DB = db; postMessage({ op: 'ready' }); });
    return;
  }
  if (m.op !== 'get') return;
  var k = m.key;
  if (INFLIGHT[k]) return;
  INFLIGHT[k] = 1;
  var t0 = Date.now();
  idbGet(k).then(function (c) {
    if (c && c.byteLength && !isHtml(c) && (!m.size || c.byteLength === m.size)) return { buf: c, src: 'idb' };
    return fromNet(k, m.rel, m.size);
  }).then(function (r) {
    delete INFLIGHT[k];
    postMessage({ op: 'data', key: k, buf: r.buf, src: r.src, ms: Date.now() - t0 }, [r.buf]);
  }).catch(function (e) {
    delete INFLIGHT[k];
    postMessage({ op: 'err', key: k, msg: String((e && e.message) || e), ms: Date.now() - t0 });
  });
};
`;

var OD = {
  // Versión de build: una línea al arrancar para saber QUÉ código corre
  // (adiós confusión de cachés). La manda la librería por __VC_CFG.version
  // (una sola fuente para el wasm y el JS); si no hay config, la de siempre.
  buildTag: (function () {
    try { return (globalThis.__VC_CFG && globalThis.__VC_CFG.version) || '2026-09-18-lib1'; }
    catch (e) { return '2026-09-18-lib1'; }
  })(),
  // Versión de DATOS: si cambia, se vacía la caché IDB antes de usarla.
  // Motivo: la caché va por ruta y el tamaño de un fichero regenerado puede
  // no cambiar (p.ej. sfx.SDT: 198820 B igual en vanilla y regenerado), así
  // que la copia vieja ganaba siempre y el motor seguía leyendo datos viejos
  // (tamaños/frecuencias de audio). Subir esto en cada rebuild de datos.
  // ve11 (21/09, sección 1): banco de audio con las 13 muestras del mod
  // (`Audio/sfx.SDT` ampliado a 9954 entradas + `Audio/sfx/9941..9953.mp3`).
  // ve12 (21/09, sección 1): `data/default.ide` con las frecuencias del mod
  // (10/7) tras el refuerzo temporal a 100 (`boost_veh_freq.py --revert`).
  // ve13 (21/09, sección 1): las 13 muestras del mod re-codificadas con su
  // frecuencia REAL (el banco del mod declara 36000/33000/22000 Hz, que no son
  // válidas para MP3: ffmpeg las resamplea y la SDT declaraba otra cosa ⇒ el
  // buffer sonaba hasta un 12 % rápido). Ver `tools/viceex-sfx-check.py`.
  // ve14 (26/09, plan agachado-calibrado): `anim/ped.ifp` con los dos clips del
  // sa-crouch para el agachado andando (`GunCrouchFwd`/`GunCrouchBwd`, 272 -> 274
  // animaciones, insertadas al final por `tools/ifp_add.py`). El motor R27 los
  // pide por nombre: sin esta purga el navegador seguiría con el ifp viejo y el
  // agachado se quedaría sin clip de movimiento.
  // ve1 (27/09, plan agachado-correcciones-axis E1/E2): la RAÍZ de
  // `GunCrouchFwd/Bwd` reescalada in situ con `tools/ifp_scale_root.py`
  // (±2,740 -> ±1,500 m/ciclo = 2,05 m/s: pies pegados a 1,13 m/s sin el x1,7
  // de R26c). Sin esta purga el navegador seguiría con el ifp viejo y las
  // pisadas irían a otro ritmo que el cuerpo.
  // ve15 (28/10, plan 23.0/T32): GXT de los menus del mod (Other Settings y
  // Camara en primera persona) guardados tambien en gamefiles/TEXT para que
  // un rebuild de datos no los borre; el wasm cambia camara, empujon,
  // particulas y disparo a ocupantes. Purga la cache IDB anterior.
  // ve16 (29/10, iteracion 24.3): GXT de los nombres que pedía la IDE y no
  // estaban en la GXT (`PREMIER` del Premier 6504, `AEROPL` del airtrain y
  // `RCGOBLI` del RC Goblin). Purga la cache IDB anterior.
  // ve17 (29/10, ajustes post-26): `FET_AGC`, `LG_39` y `LG_40` en las GXT
  // servidas (las pedia el menu de mando y la leyenda del mapa). Purga la
  // cache IDB anterior.
  // T53 (30/10): las 1708 geometrías del mapa/peds que declaraban más índices
  // de los que su plugin contiene (aceras, docks, terminal, edificios) quedan
  // recortadas. Purga la caché IDB para que el motor lea los DFF nuevos.
  // T65 (30/10, plan 32.0): las versiones de STRUCT de 34 geometrías de 8
  // vehículos no cuadraban con su cuerpo (12 bytes de surface props) y el motor
  // no encontraba el MATLIST: el modelo fallaba en bucle y acababa en
  // "memory access out of bounds". Se repara el campo de versión y se
  // reempaqueta el .dir (faggio y kaufman encogieron). Purga la caché IDB.
  // T66 (30/10, plan 34.0): las 30 geometrias de intermitente que escribio el
  // parcheador guardaban todas las cabeceras de malla primero y luego todos los
  // indices. librw las lee intercaladas, asi que tomaba indices por materiales
  // (`MESHBADMAT`), el modelo fallaba y el clump a medias reventaba en
  // `Frame::updateObjects`. Se reordena la malla (mismo tamano) y se corrige el
  // generador. Purga la cache IDB.
  // T103 (3/11, plan 42.1): 66 DFF de vehículo reconstruidos desde la base
  // limpia con la regla de hue (460 celdas: 17 nodos borrados, 4 restaurados del
  // mod, 58 ámbar real y 88 propios en su color) y `models/gta3.dir`
  // reempaquetado: 13 entradas crecen y 25 encogen (banshee 189->93 sectores al
  // quitarle los nodos inventados de octubre). Purga la cache IDB: sin eso el
  // navegador seguiria leyendo los DFF viejos por ruta.
  // T109 (3/11, plan 42.1 paso 4 re-medido): dos evidencias nuevas deciden el
  // ambar cara a cara (rasterizado de la UV de la cara y parche cerrado en el
  // atlas), asi que 10 celdas pasan de AMBAR a PROPIO (boxville y mrwhoop
  // delante, washing/linerun/topfun detras) y 18 DFF se rehacen desde
  // `tmp/base421`. `rumpo`/`pony` recuperan su unica lente ambar real que el
  // paso anterior habia borrado, y `lovefist`/`stretch`/`manana`/`cuban` quedan
  // recortados a las caras que sostienen el parche. `gta3.dir`: 2 entradas
  // crecen (topfun 82->83, zebra 97->98) y 3 encogen (hotrina, pony, romero);
  // sello del manifiesto ded0fa884ecbcc337cc6f370311aabc7.
  // T110 (3/11, plan 42.1): NO sube el tag. Todo lo del reporte es motor: la
  // tabla de lentes se compila dentro del wasm (`src/vehicles/IndicatorLensTable.h`),
  // no se sirve, y ningun DFF/txd/img cambio. Subirlo obligaria a re-bajar todo
  // el juego por un cambio de codigo (ADR-027).
  // 45.0 T6 (5/11): SI sube. El split de la lente (T4) reescribio 27 DFF del
  // gta3.img servido, 18 de ellos con tamano nuevo, y `repack_dir.py` rehizo
  // `models/gta3.dir` (streamed y bootseed) con los sectores recalculados. Sin
  // este tag el navegador seguia usando las entradas viejas de IndexedDB y el
  // motor leia modelos truncados o desalineados.
  // 47.0 T1 (5/11): solo cambia `models/gta3.dir`, y cambia de contenido sin
  // cambiar de tamano: los 195.072 bytes son las mismas 6096 entradas con un
  // layout contiguo autoritativo (antes 367 huecos y 368 solapes entre las dos
  // cajas). El sello `__vc__` del manifiesto es el que purga la cache IDB; el
  // tag sube para que el `data=` de la traza se pueda leer por plan.
  // 48.0 T1 (6/11): el plan no cambia ningun dato servido; el tag sube porque lo
  // que cambia es como el motor cuenta el tiempo (claxon a 30 Hz logicos, rampa
  // de alpha escalada, reloj que recupera hitches, empuje ped<->coche atenuado y
  // asiento compensado por altura). Se sube tambien para que
  // `check-served-build.sh` pueda comprobar que el paquete lleva el plan dentro.
  // 49.0 T1 (6/11): sigue sin cambiar ningun dato servido. Sube el tag porque
  // el censo de logica por frame movio doce contadores al reloj logico (mando,
  // giro en el aire, streaming, suspension, camara, nado, sangre, HUD, mareo,
  // peces, audio y las gotas de lente del salpicado), dejo uniforme el asiento
  // de los NPCs y anadio la cota de texturas por TXD con las trazas del Hunter.
  // 46.0 T6 (6/11): SI sube. `gen_lens_atlas.py` inyecta en 17 TXDs del
  // gta3.img servido un atlas derivado `<atlas>_lens` (copia del atlas del coche
  // con las 925 texelas sin matiz del footprint de las lentes re-tingidas al
  // matiz templado medido en ese mismo footprint, conservando la luminancia); el
  // motor lo monta solo en el flanco encendido del parpadeo. Los 17 crecen de
  // tamano (+136 KB DXT1) y `repack_dir.py` rehizo `models/gta3.dir` (streamed y
  // bootseed, mirror byte a byte) con los sectores recalculados. Purga la cache
  // IDB: sin esto el navegador seguiria abriendo los TXDs viejos, sin lente.
  // 51.0 T6 (7/11): SI sube. Los seis `TEXT/*.gxt` llevan las etiquetas nuevas del
  // menu de controles (FEC_LTS/FEC_RTS/FEC_EML dicen intermitente izquierdo /
  // intermitente derecho / luces de parqueo en los 6 idiomas; cada fichero crece
  // ~118 bytes, `stage_bootseed.py` los volco en el `.data` antes de enlazar y
  // TABL sigue OK con sus 78 misiones). El motor ademas achica el disco del
  // destello, pinta el ambar medido de la captura del mod, restringe la IA de
  // cruce del trafico y conserva el intermitente latcheado al bajarse. Sucesor en
  // NOMBRE de `2026-11-07-43.1-T1`: el contenido del 43.1 sigue en el arbol y en
  // esta build. Purga la cache IDB: sin eso el navegador abriria el GXT viejo.
  // 55.0 (8/11): REVERTIDO por el jugador («los cambios que hiciste son
  // invenciones, te pido revertirlos»). La relajacion de las tres compuertas de
  // `indicator_lens45.py` hizo pintar al benson las marcas laterales del
  // parachoques en vez de la lente que esta debajo del faro, y el parche se quito
  // (`benson.dff` restaurado byte a byte desde el respaldo del 3/11, tabla de
  // lentes de vuelta a 262 filas). Vuelve como plan 56.0 con la deteccion
  // 56.0 (8/11) T3b: el DATO vuelve a cambiar. La deteccion de lentes tenia dos
  // ciegos de muestreo (UV recortadas en V y el byte alfa basura de las texturas
  // con hasAlpha=0) ya corregidos en la herramienta; con la medida buena se
  // regenero `IndicatorLensTable.h` (262 -> 320 filas, 190 -> 204 nodos con lente
  // ambar, 0 nodos que pierdan su ambar) y se rehiciéron los nodos de los modelos
  // aprobados por el jugador: boxville, mule, trash y pizzaboy con sus cuatro
  // esquinas, fbiranch, esperant y fbicar delante. Esperant sube a 86 sectores y
  // fbicar baja a 93, asi que `gta3.dir` esta reempaquetado y las tres cajas
  // sincronizadas -tambien el `trash.dff` de `bootseed/`, que se habia quedado
  // con la version del 55.0 revertido mientras `streamed/` tenia la original-.
  // Sucesor en NOMBRE de `2026-11-08-53.0-T1`: su contenido (los GXT de opciones
  // 56.0 T8 (8/11): REVERTIDA la tanda de direccionales. El jugador lo dice con
  // sus dos capturas del mule y del fbiranch: «te pedi unicamente que activaras
  // los focos y las luces intermitentes de cada uno de esos, no inventar unos
  // nuevos ni cambiar sus formas o tamanos». Se devolvieron los nueve DFF
  // (fbiranch, boxville, mule, trash, esperant, fbicar, pizzaboy, idaho, stinger)
  // a su estado previo en las tres cajas y la tabla de lentes a las 262 filas
  // servidas, anadiendo UNICAMENTE las dos filas del benson, que es el unico que
  // el jugador confirma funcionando. Prueba dura de que el arbol de datos volvio
  // al punto aprobado: el sello del manifiesto es de nuevo
  // `60a0c0a3f340f41b170e4b6b21982b0d`, el mismo que dejo el revert del 55.0.
  // Sucesor en NOMBRE de `2026-11-08-56.0-T7` y `2026-11-08-56.0-T3` (las dos
  // tandas revertidas, cuyo contenido ya no esta en el arbol) y de
  // `2026-11-08-53.0-T1` (los GXT de opciones extra, intactos). Purga IDB.
  // 58.0 T3 (8/11): los TXD de esperant/boxville/fbiranch/mule/trash reciben su
  // atlas derivado <atlas>_lens (lentes ambar de carroceria, plan 58.0 T3/T4).
  // Cambian los datos servidos -> purga IDB. Sucesor en NOMBRE de
  // `2026-11-08-56.0-T8` (la tanda revertida, cuyo contenido sigue fuera del
  // arbol); ese rotulo se conserva aqui para que su marca de check-served-build
  // siga encontrandolo en el paquete.
  // 58.0 T4 (8/11): el material de lente de carroceria se parte en slot
  // izquierdo y derecho (split_lens_material.py) en esperant/boxville/mule/
  // trash (chassis_hi) y fbiranch (chassis_hi detras, bump_front_hi_ok
  // delante): cero cambio de vertices/posiciones, solo indice de material por
  // triangulo y dos copias identicas del material. BodyLensTable.h regenerada:
  // cada fila pinta SOLO el slot de su costado (giro a la izquierda ya no
  // enciende el cristal derecho). Los TXD no cambian; cambian los DFF y el
  // gta3.dir -> purga IDB. Sucesor en NOMBRE de `2026-11-08-58.0-T3` (sus atlas
  // derivados siguen servidos intactos); ese rotulo se conserva aqui para que
  // su marca de check-served-build siga encontrandolo en el paquete.
  // 58.1 T1 (9/11): cambian los TXD de topfun/rumpo/pony (atlas derivado de cara
  // completa <tex>_lens: la direccional delantera enciende entera, no solo el
  // hilo), mrwhoop (atlas derivado de carroceria sobre el faro) y trash (se
  // retira el atlas inventado del porton); zebra.dff re-parenta el clon de
  // pintura fuera del nodo indicator y mrwhoop.dff parte el material de la lente
  // por costado -> purga IDB. Sucesor en NOMBRE de `2026-11-08-58.0-T4` (sus
  // atlas de esperant/boxville/fbiranch/mule siguen servidos intactos); ese
  // rotulo se conserva aqui para que su marca de check-served-build siga
  // encontrandolo en el paquete.
  // 59.0: cambia trash.txd (atlas `trash868bit128_lens` re-escrito a ARGB8888
  // con la cara entera de las direccionales traseras) y con el gta3.dir
  // reempaquetado (los TXD que el plan 59.0 heredo reescritos crecieron de
  // sectores: boxville, cabbie, enforcer, fbiranch, manana, mrwhoop...). El
  // que purga de verdad es el sello de contenido del manifiesto (ADR-027);
  // este rotulo solo se reporta. dataTag anterior: `2026-11-09-58.1-T1`.
  // 61.0 T2: TXD del banshee convertido a 32 bits; el sello del manifiesto
  // tambien cambia, que es lo que purga la cache IDB (ADR-027).
  // 61.0 T3: el cuerpo del banshee lleva marcador de pintura en su material
  // dominante (chassis_hi); el dataTag sube para purgar la IDB con el DFF nuevo.
  // 61.0 T4: el cuerpo usa la textura banshee86body128a; el dataTag sube para
  // purgar la IDB con el DFF nuevo (386685 bytes).
  // 62.0 T1: DFF de mrwhoop, rumpo, pony, topfun, boxville, esperant, fbiranch y
  // fbicar con el cristal en slot propio; cambian tambien las dos tablas de lentes.
  // dataTag anterior: `2026-11-09-62.0-T1`.
  // 61.0 T5: el chassis_hi reparte el atlas banshee868bit128 (1946 tris);
  // el dataTag sube para purgar la IDB con el DFF nuevo (386693 bytes).
  // dataTag anterior: `2026-11-09-62.0-T2`.
  // 61.0 T6: el chassis_hi pasa al atlas completo (3956 tris); la textura de
  // puertas `banshee86body128a` sale del chasis; DFF 386685 bytes.
  // 62.1 T1: fbicar revertido y el nodo solo se enciende con fila ambar medida;
  // el faro blanco del mrwhoop y los pilotos de rumpo/pony/topfun dejan de
  // recibir ambient. dataTag anterior: `2026-11-09-62.0-T3`.
  // 62.2 T1: T7 alinea las UVs del atlas del Banshee contra el PC y reconstruye
  // los grupos de malla desde el STRUCT. dataTag anterior: `2026-11-09-62.1-T1`.
  // 62.3 T1: T8 copia las UVs REALES de los triángulos PC (cuerpo y atlas) al
  // chassis_hi Xbox; 0 degenerados. dataTag anterior: `2026-11-09-62.2-T1`.
  // 63.0 T1 (10/11): T9 instala el Banshee COMPLETO de The Hero 1:1: DFF
  // 342016 B, 33 frames/16 atomics y chassis_hi 3956 tris con los 9 materiales
  // Xbox repartidos; TXD 1552204 B, 8 texturas 32 bits. Sustituye al T8 y purga
  // la IDB (el DFF/TXD y el .dir cambian de tamaño/sectores).
  // 62.2 T2: cristal medido por matiz+saturacion (pony/topfun/esperant ganan
  // las caras que el 20 % dejaba fuera) y fbicar declarado con las mismas
  // caras y sitios del washing (6 delante 407,8 cm2, 7 detras 228,0 cm2).
  // Etiqueta: la lane del banshee uso 62.2-T1, 62.3-T1 y `2026-11-10-63.0-T1`
  // para su 61.0; se conservan aqui para que el paquete siga dando su marca.
  // 62.3 T2: huella del derivado = las caras que DECLARA la fila (no solo las
  // del nodo): rumpo/pony/topfun/esperant pasan del 22-35 % al 100 % de
  // cobertura por cara, 527 texelas, fuera=0. Se conserva el literal
  // `2026-11-10-63.0-T1` y `2026-11-10-63.1-T1` de la lane del banshee, y la
  // propia `2026-11-09-62.2-T2`, que el checker exige ver en el paquete.
  // 63.2 T1 (10/11): Banshee Xbox con la capa de reflejo del Banshee PC
  // (bansheespeca + coeficiente 1.0 en pintura y cristales, DFF 344176 B,
  // TXD 1574200 B); purga la IDB con los datos nuevos.
  // dataTag anterior: `2026-11-09-62.3-T2`.
  // 60.0 §13 (14/11): Stinger faro rojo pintado ambar en el atlas + traseras al
  // fallback rojo; firetruk con las piezas extradetails/servicelights_0 del PC
  // (canon visible otra vez, DFF 379475 B, TXD 2264412 B); y la tanda 2 del
  // plan 60 (vicechee, admiral, cabbie, landstal, hotrinb, police, ambulan)
  // con la capa de reflejo y sus lentes ambar. Sube por la IDB: cambian los
  // DFF/TXD y el .dir (están en streamed+gamefiles y en bootseed horneado).
  // Se conserva el literal `2026-11-13-66.2-T1` (purga de la IDB del 66.2).
  // dataTag anterior: `2026-11-14-66.3-T1`.
  // 60.1: lentes, barras, luces de servicio y cañon nuevos en 8 modelos.
  // 60.3 T1: revision del jugador. Stinger sin indicadores delanteros y lente
  // pintada en ambar; police con una sola barra (policebar_hi) y placa con el
  // RGB del PC; vicechee con la farola fija (vicebar1/2/3_hi); panes window_*
  // del PC en vicechee/police/admiral/ambulan y vidrio fundido fuera de las
  // puertas; firetruk sin el watercannon inventado.
  // 60.3 T2: los DFF de vicechee/police/admiral/ambulan salieron con el STRUCT
  // de geometria 12 bytes corto (strip_glass) y el police no cargaba (`buf
  // falla police`): regenerados y validados chunk a chunk. dataTag anterior:
  // `2026-11-14-60.1-T1`.
  // 60.5 T1: los 11 vuelven a carroceria PC + piezas Xbox (interior asientos/
  // salpicadero/paneles, motor, bajos, escape); sin lentes ni matriculas Xbox;
  // volante propio (nunca el negro). dataTag anterior: `2026-11-15-60.4-T4b`.
  // 60.7 T1: la patrulla con una sola barra de sirena (fuera la V duplicada) y
  // ventanilla de puerta con su estado danado antes de romperse. dataTag
  // anterior: `2026-11-15-60.6-R2`.
  // 60.8 T1: el Banshee vuelve a base PC (el restore era el DFF Xbox completo y
  // el TXD faltaba) y estrena interior/motor/bajos/vano Xbox, con V12 y
  // belowcars anadidos al atlas PC y paneles _dam deformados. dataTag anterior:
  // `2026-11-16-60.9-T6`.
  // 60.8 T14: los 9 restantes estrenan interior/motor/bajos Xbox y paneles de
  // puerta; los TXD base de 8 bits se convierten a 32 bits. dataTag anterior:
  // `2026-11-16-60.9-T13`.
  // 60.10 T1: correccion del lote de los 9 (metodo admiral): exterior PC intacto
  // (morro/cola/luces/parachoques), od_interior solo cabina sin solapes, vidrios
  // y paneles _dam rehechos. dataTag anterior: `2026-11-17-60.9-T17`.
  // 60.10 T2: vidrios al pase transparente, od_interior antes de las grietas,
  // sirena vicechee y piel de puerta landstal conservadas. dataTag anterior:
  // `2026-11-16-60.10-T1`.
  // 60.10 T3: correcciones de pilotos sin plan: vidrios con tinte uniforme,
  // sirena vicechee por componentes completos sin cortes, y un solo volante movil
  // (fijo Xbox retirado / anclaje del procedural <=100). dataTag anterior:
  // `2026-11-16-60.10-T2`.
  // 60.10 T4: vicechee farola de sirena permanente (od_siren) y pareja de
  // retrovisores permanente (od_mirrors); extra1/extra2 vaciados para que el
  // sorteo aleatorio de componentes no deje el coche con 1 retrovisor o sin
  // farola. dataTag anterior: `2026-11-17-60.10-T3`.
  // 60.10 T6: chorro del firetruk anclado a la boca real del dispersor (nodo
  // bump_rear pivotante, y1.80/z2.00) y REVERTIDO el trim de molduras de T5
  // (refutado: era el respaldo interior de la ventanilla que admiral conserva;
  // quitarlo flotaba el material de la otra puerta sobre el vidrio roto).
  // 60.10 T7: la mancha sobre la luna rota era el chassis PC (cartas planas
  // opacas tapando el hueco: respaldo negro None + body inboard), no el
  // od_interior. Se retiran por componente en coches con luna lateral; admiral
  // no las tiene. vicechee: fuera el cabezal de retrovisor duplicado del chassis
  // (quedaban 3 en algunos angulos) => exactamente 2 (od_mirrors).
  // dataTag anterior: `2026-11-17-60.10-T6`.
  // 60.10 T8: la retirada de cartas del chassis de T7 leia el buffer original y
  // deshacia la retirada de cabina (cuerpo PC entero otra vez); ahora va en la
  // misma pasada sobre el clump clonado. dataTag anterior:
  // `2026-11-17-60.10-T7`.
  // 60.10 T13: vicechee, vuelve el marco del parabrisas (el filtro de lunas lo
  // tomaba por respaldo) y fuera la pala vertical del pilar A que se veia como
  // tercer retrovisor encima del del conductor. dataTag anterior:
  // `2026-11-17-60.10-T12`.
  // 61.0 T1: la textura paletizada del rines (wheel_lightvan64back) pasa a 32
  // bits sin comprimir. dataTag anterior: `2026-11-18-61.0`.
  dataTag: '2026-11-19-77.4-T1',
  manifest: null,
  db: null,
  inflight: {},
  pending: {},
  misses: [],
  // Consola con detalle (sin query param: siempre verboso).
  verbose: true,
  // Rutas configurables: la librería (web/lib/index.js) deja window.__VC_CFG
  // ANTES de cargar reVC.js, así el mismo binario sirve para cualquier host
  // (/streamed, CDN, subcarpeta...). Sin config, los valores de siempre.
  cfg: (function () {
    const d = { streamedUrl: '/streamed/', manifestUrl: '/manifest.json', odtraceUrl: '/odtrace', idbCapMB: 900, warmMB: 128, worker: true };
    try {
      const c = (typeof globalThis !== 'undefined' && globalThis.__VC_CFG) || {};
      if (c.streamedUrl) d.streamedUrl = c.streamedUrl;
      if (c.manifestUrl) d.manifestUrl = c.manifestUrl;
      if (c.odtraceUrl !== undefined) d.odtraceUrl = c.odtraceUrl;
      if (c.idbCapMB !== undefined) d.idbCapMB = Number(c.idbCapMB) || 0;
      if (c.warmMB !== undefined) d.warmMB = Number(c.warmMB) || 0;
      if (c.worker !== undefined) d.worker = !!c.worker;
    } catch (e) {}
    for (const k of ['streamedUrl']) if (d[k] && d[k].slice(-1) !== '/') d[k] += '/';
    return d;
  })(),
  sleep(ms) { return new Promise((r) => setTimeout(r, ms)); },
  // Flush IDBFS serializado (F2): solapar syncfs atasca la cola IDB
  // ("N operations in flight") y nada completa. Si hay uno en vuelo, encolar.
  // Con origen (save/settings/quit) deja línea en consola; el resto va
  // silencioso. Los saves son acciones raras del usuario: 2 líneas no es spam.
  _syncFsBusy: false,
  _syncFsPending: false,
  _ufTimer: null,
  syncUserfiles(src) {
    if (OD._syncFsBusy) { OD._syncFsPending = true; if (src) odLog('[od] flush ' + src + ': en cola'); return; }
    OD._syncFsBusy = true;
    if (src) odLog('[od] flush ' + src + '...');
    try { FS.syncfs(false, () => { OD._syncFsBusy = false; if (src) odLog('[od] flush ' + src + ' ok'); if (OD._syncFsPending) { OD._syncFsPending = false; OD.syncUserfiles('pendiente'); } }); }
    catch (e) { OD._syncFsBusy = false; if (src) odLog('[od] flush ' + src + ' ERROR: ' + e); }
  },
  // Estilo dos.zone: toda escritura bajo /userfiles programa un flush con
  // antirrebote (sin ventana de 30 s y sin solapes por construcción).
  noteUserfilesWrite(path) {
    if (!path || path.indexOf('/userfiles') !== 0) return;
    if (OD._ufTimer) clearTimeout(OD._ufTimer);
    OD._ufTimer = setTimeout(() => { OD._ufTimer = null; OD.syncUserfiles(); }, 2500);
  },
  // Caché MEMFS con desalojo: solo lo traído por OD es desalojable (el
  // paquete inicial nunca). Lo desalojado se re-descarga (IDB lo sirve sin
  // red). Sin esto la sesión crece sin cota y la pestaña muere (~2.4 GB).
  live: new Map(),
  liveBytes: 0,
  // Tope de descargas SIMULTANEAS (21/10). Al cargar el mundo el motor
  // pide mas de 1400 ficheros de golpe y cada uno era un fetch en
  // paralelo: el heap de JS se iba a 900 MB, el GC tardaba 11 s y el
  // navegador apagaba el WebGL (pantalla negra con el sonido de fondo).
  // Con el tope, las descargas van a rafagas de 48 y el pico baja mucho.
  dlMax: 48,
  dlActive: 0,
  dlQueue: [],
  dlSlot() {
    if (OD.dlActive < OD.dlMax) { OD.dlActive++; return Promise.resolve(); }
    return new Promise((res) => OD.dlQueue.push(res));
  },
  dlRelease() {
    if (OD.dlActive > 0) OD.dlActive--;
    const next = OD.dlQueue.shift();
    if (next) { OD.dlActive++; next(); }
  },
  // 24/10 (dieta por los cuelgues): los topes anteriores (CAP 360, blando 180,
  // HEAP_HI 700) son los que dejaron morir la pestaña tres veces. Medido en las
  // sesiones de 2026-10-24 con la build 17.0-T9 (tag ODCAP de la traza): MEMFS
  // 436 MB con heap JS 551 MB, y MEMFS 517 MB con heap JS 624 MB, siempre con
  // el heap wasm clavado en 512 MB y SIN desalojar nunca (`evict=0/0MB`). La
  // traza se corta en mitad de un frame a 60 fps sin JSERR, sin ENGERR y sin
  // pérdida de WebGL: eso es el proceso del renderer muriendo por memoria, no
  // una excepción del motor. El paquete inicial (~164 MB) no es desalojable, así
  // que el techo real de MEMFS es 164 + CAP.
  //
  // 25/10 (tirones): con CAP 176 y gracia 20 s el working set se desalojaba
  // mientras se conducía (2.877 ficheros en 2 min) y cada re-petición suspende el
  // tick por una descarga on-demand: `strm` llegó a 2.159 ms en un solo frame y
  // `JS ASYNC` a 443 ms en un lote. El heap wasm estaba pidiendo 512 MB y usando
  // 64, así que ahora arranca en 384 MB (build.sh, crece si hace falta) y esos
  // 128 MB se le dan al caché: CAP 240 y gracia 45 s. El techo de MEMFS sube a
  // ~404 MB y el total del renderer se queda donde estaba.
  // 26/10 (thrash medido): el heap de V8 en partida vive en 420-440 MB de forma
  // normal (traza, buffers de descarga, el mapa de ficheros). Con HEAP_HI=420 la
  // vigilancia disparaba cada 8 s: el log lo enseña literal — `ODMEM heap=430MB
  // desalojo live=166->96MB` y dos veces más — tirando a la basura modelos que
  // el motor estaba usando, que volvian a pedirse al frame siguiente. Eso es el
  // "carga de a poco" al salir de un interior y los tirones: un bucle
  // desalojo -> re-peticion -> descarga. El limite que si mataba la pestaña eran
  // 551-624 MB de heap CON 436-517 MB de MEMFS; ahora MEMFS vive en ~300 MB, asi
  // que el aviso sube a 520 MB, exige tres medidas seguidas (24 s) y desaloja
  // solo hasta el 70% del tope en vez de dejarlo en 96 MB.
  // 19/11 (escalón +16 MB, sesión 77.0 de las 19:33): el pool tocó techo en
  // partida (`live=238MB` de 240) y ahí empezaban los desalojos por gracia de
  // ficheros que el motor estaba usando. Un escalón pequeño y reversible: el
  // techo de MEMFS pasa de ~404 a ~420 MB. Si el móvil mata la pestaña, volver
  // a 240 antes que tocar HEAP_HI.
  CAP: 256 * 1048576,
  GRACE_MS: 45000,
  HEAP_HI: 520 * 1048576,
  CAP_SOFT: 96 * 1048576,
  heapOver: 0,
  evicted: 0,
  evictedBytes: 0,
  fetchedBytes: 0,
  fetchedCount: 0,
  idbHits: 0,
  touch(path, size) {
    const e = OD.live.get(path);
    if (e) { e.t = Date.now(); return; }
    OD.live.set(path, { s: size, t: Date.now() });
    OD.liveBytes += size;
    OD.evict();
  },
  evict(force, target) {
    const cap = typeof target === 'number' ? target : (force ? OD.CAP_SOFT : OD.CAP);
    if (OD.liveBytes <= cap) return;
    const now = Date.now();
    // Doble del tope: desalojo duro (sin periodo de gracia). MEMFS mantiene
    // vivos los streams abiertos aunque se deslinkee el nombre; reabrir
    // re-descarga (IDB lo sirve sin red). La alternativa es morir por OOM.
    const hard = force || OD.liveBytes > 2 * OD.CAP;
    const order = [...OD.live.entries()].sort((a, b) => a[1].t - b[1].t);
    for (const [p, e] of order) {
      if (OD.liveBytes <= cap) break;
      if (!hard && now - e.t < OD.GRACE_MS) continue;
      try { FS.unlink(p); } catch (err) { OD.live.delete(p); OD.liveBytes -= e.s; continue; }
      OD.live.delete(p);
      OD.liveBytes -= e.s;
      OD.evicted++;
      OD.evictedBytes += e.s;
    }
    if (OD.verbose && OD.evicted % 50 === 1) odLog('[od] desalojo total=' + OD.evicted + ' live=' + Math.round(OD.liveBytes / 1048576) + 'MB');
  },
  // Dirs que el motor espera que existan (statvfs, scans). Se crean vacíos.
  // OJO: jamás pre-crear un dir que colisione con un ARCHIVO necesario
  // (anim/cuts.img lo hizo: fcaseopen encontraba el dir vacío en vez del
  // .img y la cinemática salía negra). Solo models/gta3.img (backend suelto).
  dirs: [
    '/anim', '/Audio', '/data', '/models', '/models/gta3.img', '/TEXT',
    '/txd', '/neo', '/skins', '/userfiles',
  ],

  norm(p) {
    return String(p).replace(/\\/g, '/').replace(/^\.\//, '').replace(/^\/+/, '').toLowerCase();
  },

  // Censo MEMFS (ficheros y MB) con caché de 30 s para el heartbeat.
  _census: { t: 0, n: 0, mb: 0 },
  census() {
    const now = Date.now();
    if (now - OD._census.t < 30000) return OD._census;
    let n = 0, b = 0;
    const per = {};
    try {
      const stack = ['/anim', '/Audio', '/data', '/models', '/TEXT', '/txd', '/neo', '/skins', '/userfiles'];
      while (stack.length) {
        const dir = stack.pop();
        let ents;
        try { ents = FS.readdir(dir); } catch (e) { continue; }
        for (const e of ents) {
          if (e === '.' || e === '..') continue;
          const p = dir + '/' + e;
          try {
            const st = FS.stat(p);
            if (FS.isDir(st.mode)) stack.push(p);
            else {
              n++; b += st.size;
              const top = '/' + p.slice(1).split('/')[0];
              per[top] = (per[top] || 0) + st.size;
            }
          } catch (ee) {}
        }
      }
    } catch (e) {}
    const big = [];
    try {
      const stack2 = ['/anim', '/Audio', '/data', '/models', '/TEXT', '/txd', '/neo', '/skins'];
      while (stack2.length) {
        const dir = stack2.pop();
        let ents;
        try { ents = FS.readdir(dir); } catch (e) { continue; }
        for (const e of ents) {
          if (e === '.' || e === '..') continue;
          const p = dir + '/' + e;
          try {
            const st = FS.stat(p);
            if (FS.isDir(st.mode)) stack2.push(p);
            else if (st.size > 20 * 1048576) {
              // Prueba de integridad: ¿se puede abrir y leer de verdad?
              // Si el tamaño miente (metadatos corruptos), el read falla.
              let probe = 'n/a';
              try {
                const s = FS.open(p, 'r');
                const buf = new Uint8Array(16);
                const n = FS.read(s, buf, 0, 16, 0);
                FS.close(s);
                probe = 'read=' + n + ':' + Array.from(buf.slice(0, Math.min(n, 8)))
                  .map((b) => b.toString(16).padStart(2, '0')).join('');
              } catch (ee) { probe = 'ERROR:' + (ee && ee.message || ee); }
              big.push(p + ':' + Math.round(st.size / 1048576) + 'MB[' + probe + ']');
            }
          } catch (ee) {}
        }
      }
    } catch (e) {}
    OD._census = { t: now, n, mb: Math.round(b / 1048576) };
    OD._census.bigN = big.length;
    OD._census.big = big.slice(0, 25).join(' ');
    OD._census.live = Math.round(OD.liveBytes / 1048576);
    OD._census.ev = OD.evicted;
    OD._census.evmb = Math.round(OD.evictedBytes / 1048576);
    OD._census.fmb = Math.round(OD.fetchedBytes / 1048576);
    OD._census.idb = OD.idbHits + ((OD.wkStats && OD.wkStats.idb) || 0);
    OD._census.per = Object.entries(per).map(([k, v]) => k + ':' + Math.round(v / 1048576)).join(' ');
    OD._census.ufs = Math.round((per['/userfiles'] || 0) / 1048576);
    OD._census.root = Math.round((per['/'] || 0) / 1048576);
    return OD._census;
  },

  // Emisoras del dial (orden retail, para "la siguiente").
  radios: ['WILD', 'FLASH', 'KCHAT', 'FEVER', 'VROCK', 'VCPR', 'ESPANT',
           'EMOTION', 'WAVE'],

  // F5c (fluides-v2): NADA de prefetch masivo (las 9 = 270 MB que ahogan el
  // cap y provocan el thrash medido en lean1: 1683 ficheros re-traídos).
  // Sólo vive en MEMFS la emisora que suena; al sonar una, se precarga LA
  // SIGUIENTE del dial en background (cambio de emisora sin freeze). Al
  // arrancar, la última usada (localStorage) a los 15 s.
  currentStation: null,
  _nextTimer: null,
  noteStation(station) {
    OD.currentStation = station;
    try { localStorage.setItem('vc_lastStation', station); } catch (e) {}
    if (OD._nextTimer) clearTimeout(OD._nextTimer);
    const i = OD.radios.indexOf(station);
    if (i < 0) return;
    const next = OD.radios[(i + 1) % OD.radios.length];
    OD._nextTimer = setTimeout(() => {
      OD._nextTimer = null;
      // R2: la precarga de LA SIGUIENTE emisora no puede caer en una carga
      // bloqueante del motor (ahí el motor no reintenta y el frame se queda
      // parado) ni escribir 30 MB en MEMFS sin que nadie los haya pedido (era
      // una copia en el hilo del juego en un momento arbitrario). Se pide al
      // worker y sus bytes quedan retenidos hasta que el motor pida el fichero.
      let block = 0;
      try { block = window.__vcODBlock | 0; } catch (e) {}
      if (block > 0) { OD.adfTrace(next, 0, 0, 'precarga=0 motivo=block'); return; }
      if (OD.wk) {
        const canon = '/Audio/' + next + '.ADF';
        const h = OD.manifest && OD.manifest[OD.norm(canon)];
        OD.wkAsk(canon, h);
        // Con un "esperador" el worker NO escribe MEMFS: se retiene (wkPut) y se
        // escribe sólo cuando el motor pida el fichero de verdad.
        OD.wkWait(canon).then((buf) => { if (buf) OD.wkPut(canon, buf); }).catch(() => {});
        OD.adfTrace(next, 0, 0, 'precarga=1 retenida=1');
      } else {
        OD._ensure('/Audio/' + next + '.ADF').catch(() => {});
        OD.adfTrace(next, 0, 0, 'precarga=1');
      }
    }, 3000);
  },

  async prefetchLastStation() {
    let last = null;
    try { last = localStorage.getItem('vc_lastStation'); } catch (e) {}
    if (last && OD.radios.includes(last)) {
      // _ensure directo (no dispara noteStation).
      try { await OD._ensure('/Audio/' + last + '.ADF'); } catch (e) {}
    }
  },

  // Una línea al odtrace.log: deja constancia de qué código y qué datos corren
  // en cada sesión (sin esto, una traza sin el tag no se puede interpretar).
  // F4c: NO usar XHR SÍNCRONO. Bloquea el hilo del juego la ida y vuelta
  // completa al servidor, y la traza ASYNC sale una vez por segundo: era el
  // único trabajo que robaba milisegundos a un frame de cada segundo (marco
  // de medición del tirón). Se encola como las trazas de C++ (window.__odq,
  // que main.js manda por lotes en segundo plano). Fallback: XHR síncrono
  // solo si no hay cola (página sin main.js).
  trace(msg) {
    const line = 'JS ' + msg;
    try {
      if (window.__odq) { window.__odq.push(line); return; }
      if (!OD.cfg.odtraceUrl) return;
      const x = new XMLHttpRequest();
      x.open('POST', OD.cfg.odtraceUrl, false);
      x.send(line);
    } catch (e) {}
  },

  async init() {
    odLog('[build] ' + OD.buildTag + ' (si no ves este tag, recarga con Ctrl+Shift+R)');
    OD.trace('build=' + OD.buildTag + ' data=' + OD.dataTag);
    // Puente mínimo recoil (plan recoil-por-arma-y-cadencia): si la librería
    // no definió la sesión (build servido sin lib), el EM_ASM de C++ no debe
    // tirar y las líneas van a __odq con identidad no disponible. La lib lo
    // sustituye cuando existe; sin ?trace=recoil-<id> la sesión queda
    // inactiva y C++ no emite nada.
    try {
      if (!window.__odRecoilSession) window.__odRecoilSession = { active: false, id: 'unavailable' };
      if (!window.__odRecoilEmit) {
        let seq = 0;
        window.__odRecoilEmit = (ev) => {
          try {
            const line = `RECOIL sessionId=unavailable eventSeq=${++seq} build=${OD.buildTag} data=${OD.dataTag} wasm=unavailable ${String(ev).replace(/\s+/g, ' ').slice(0, 640)}`;
            (window.__odq = window.__odq || []).push(line);
          } catch (e) {}
        };
      }
    } catch (e) {}
    try { document.title = '[vc ' + OD.buildTag + '] GTA Vice City'; } catch (e) {}
    for (const d of OD.dirs) {
      try { FS.mkdirTree(d); } catch (e) {}
    }
    try {
      const r = await fetch(OD.cfg.manifestUrl, { cache: 'no-cache' });
      OD.manifest = await r.json();
    } catch (e) { OD.manifest = {}; }
    setTimeout(() => OD.prefetchLastStation(), 15000);
    try {
      OD.db = await new Promise((res, rej) => {
        // v2 añade el store 'meta' (marca de uso por fichero) para el techo LRU.
        // Las bases v1 se actualizan solas: 'files' se conserva tal cual.
        const q = indexedDB.open('vcod2', 2);
        q.onupgradeneeded = (e) => {
          const db = e.target.result;
          if (!db.objectStoreNames.contains('files')) db.createObjectStore('files');
          if (!db.objectStoreNames.contains('meta')) db.createObjectStore('meta');
        };
        q.onsuccess = (e) => res(e.target.result);
        q.onerror = (e) => rej(e);
      });
    } catch (e) { OD.db = null; }
    try { await OD.idbPurgeIfStale(); } catch (e) {}
    // El techo de la caché se revisa en segundo plano (no retrasa el arranque)
    // y las marcas de uso se vuelcan en lotes cada 30 s junto con la lista de
    // arranque en caliente. En 'pagehide' se intenta el último vuelco.
    setTimeout(() => { OD.idbTrim('init').catch(() => {}); }, 12000);
    setInterval(() => { OD.idbFlushTouches().catch(() => {}); OD.warmPersist(); }, 30000);
    try { addEventListener('pagehide', () => { OD.warmPersist(); OD.idbFlushTouches(); }); } catch (e) {}
    OD.wkStart();          // Nivel 1: el trabajo de carga, en un hilo aparte
    OD.hookWorldLoad();
    // F4a: una línea ASYNC por segundo con las suspensiones que atendió el
    // pre-js (n y wall-ms). Emparejar con FPSLOG maxdelta del mismo segundo.
    setInterval(() => {
      try {
        if (OD.asyncN > 0) OD.trace('ASYNC n=' + OD.asyncN + ' ms=' + Math.round(OD.asyncMs));
        OD.asyncN = 0; OD.asyncMs = 0;
      } catch (e) {}
    }, 1000);
    // Telemetría de memoria cada 30 s (tag ODCAP): working set real de MEMFS
    // (census con caché 30 s) + heap JS de V8. Para ajustar CAP y la dieta
    // con datos, no a ojo.
    setInterval(() => {
      try {
        const c = OD.census();
        let js = '';
        if (performance.memory) js = ' js=' + Math.round(performance.memory.usedJSHeapSize / 1048576) + 'MB';
        const per = c.per ? ' desglose=' + c.per : '';
        OD.trace('ODCAP memfs=' + c.mb + 'MB files=' + c.n + ' live=' + c.live + 'MB' +
          ' evict=' + c.ev + '/' + c.evmb + 'MB fmb=' + c.fmb + 'MB idb=' + c.idb + js + per);
      } catch (e) {}
    }, 30000);
    // Vigilancia de memoria: con el heap JS por encima de HEAP_HI el
    // navegador puede matar la pestana. Se desaloja el pool por debajo
    // del tope blando (sin periodo de gracia) para bajar el pico. Solo
    // actua bajo presion; el pool vuelve a crecer solo hasta CAP.
    setInterval(() => {
      try {
        if (!performance.memory) return;
        const js = performance.memory.usedJSHeapSize;
        OD.heapOver = js > OD.HEAP_HI ? (OD.heapOver | 0) + 1 : 0;
        // Tres medidas seguidas (24 s) por encima del aviso: un pico de GC o un
        // lote de descargas no puede vaciar la cache de modelos.
        // 19/11 (medido en la sesión 77.0 de las 19:33): con el heap JS en
        // 562-604 MB este aviso desalojó 6.100 ficheros (live 233->168 MB) y el
        // heap NO bajó ni un MB (604 antes y después). Lo que ocupa esos MB no es
        // el pool de modelos sino la página (traza y buffers), así que el desalojo
        // solo servía para que el motor volviera a pedir 65 MB de golpe al segundo
        // siguiente (`evict=6419/222MB`) con sus paradas de frame. Ahora se avisa
        // y NO se toca el pool mientras esté por debajo de CAP: solo se recorta
        // con presión real (el pool pasado de tope), que es el caso para el que
        // se escribió.
        const target = OD.CAP * 0.7;
        if (OD.heapOver >= 3 && OD.liveBytes > OD.CAP) {
          const before = OD.liveBytes;
          OD.evict(true, target);
          OD.trace('ODMEM heap=' + Math.round(js / 1048576) + 'MB desalojo live=' +
            Math.round(before / 1048576) + '->' + Math.round(OD.liveBytes / 1048576) + 'MB');
          OD.heapOver = 0;
        } else if (OD.heapOver === 3) {
          OD.trace('ODMEM heap=' + Math.round(js / 1048576) + 'MB aviso sin desalojo live=' +
            Math.round(OD.liveBytes / 1048576) + 'MB cap=' + Math.round(OD.CAP / 1048576) + 'MB');
        }
      } catch (e) {}
    }, 8000);
    // Integridad de los ficheros gordos (>20 MB): son los que deciden el peso
    // en memoria y donde una entrada mentirosa se nota (cinemáticas, emisoras,
    // cuts.img). Cada línea ODBIG trae el tamaño y una prueba de lectura real
    // de 1 B: 'read=n:hex' es sano; 'ERROR:...' son metadatos rotos. Así se
    // distingue "no se cargó" de "se cargó y no se puede leer".
    setInterval(() => {
      try {
        const c = OD.census();
        // Siempre una línea, también cuando NO hay ninguno: "no hay fichero
        // gordo en memoria" es justo el dato que dice que cuts.img (115 MB) no
        // llegó (escena de cinemática vacía) en vez de "llegó y no se lee".
        OD.trace('ODBIG n=' + (c.bigN || 0) + ' ' + (c.big || ''));
      } catch (e) {}
    }, 60000);
    // Bucles primero (motor, sirenas, ambientes): son los que suenan en cuanto
    // conduces y no pueden permitirse una descarga/decodificación a mitad.
    setTimeout(() => { OD.prefetchAnnouncements(); }, 4000);
    setTimeout(() => { OD.prefetchLoops(); }, 6000);
  },

  // Trae en silencio las muestras con puntos de loop (las lee de la propia
  // tabla SDT que ya está en el FS virtual). Evita el tirón de la primera vez
  // que suena un motor.
  async prefetchAnnouncements() {
    for (const p of ['/Audio/bclosed.mp3', '/Audio/bopen.mp3']) {
      try { await OD._ensure(p); } catch (e) {}
    }
  },
  async prefetchLoops() {
    try {
      let p = '/Audio/sfx.SDT';
      if (!OD.exists(p)) {
        try { p = await OD.ensure('/Audio/sfx.SDT'); } catch (e) { p = null; }
      }
      if (!p) { odLog('[od] loops: sin SDT'); return; }
      const u8 = FS.readFile(p, { encoding: 'binary' });
      const n = Math.floor(u8.length / 20);
      const list = [];
      for (let i = 0; i < n; i++) {
        const o = i * 20;
        const size = (u8[o + 4] | (u8[o + 5] << 8) | (u8[o + 6] << 16) | (u8[o + 7] << 24)) >>> 0;
        if (!size) continue;
        const ls = (u8[o + 12] | (u8[o + 13] << 8) | (u8[o + 14] << 16) | (u8[o + 15] << 24)) >>> 0;
        const le = (u8[o + 16] | (u8[o + 17] << 8) | (u8[o + 18] << 16) | (u8[o + 19] << 24)) | 0;
        if (ls !== 0 || (le !== 0 && le !== -1)) list.push(i);
      }
      odLog('[od] samples con loop en la SDT: ' + list.length);
      let done = 0;
      for (const i of list) {
        try { if (await OD.ensure('/Audio/sfx/' + i + '.mp3')) done++; } catch (e) {}
        if (done % 20 === 0) await OD.sleep(40);
      }
      odLog('[od] bucles precargados ' + done + '/' + list.length);
    } catch (e) { odLog('[od] prefetch loops: ' + (e && e.message || e)); }
  },

  // ------------------------------------------------------------------------
  // F6b · Arranque en caliente del primer minuto
  //
  // Medido: en el primer minuto de juego hay un hipo de 20-40 ms (y picos de
  // 110-266 ms) por cada zona nueva. La causa es que CADA fichero que el motor
  // toca por primera vez cuesta una suspensión Asyncify + una lectura de la
  // caché IDB (~3,3 ms), y en una zona nueva pide ~68 por segundo (226 ms de
  // trabajo en un segundo). Ese trabajo es el mismo se haga cuando se haga:
  // lo que cambia es DÓNDE se paga. Aquí se adelanta a la pantalla de carga.
  //
  // Sin saber nada del mapa: se aprende del propio uso. Se anota lo que el
  // motor pide en la sesión y la siguiente se precarga durante la carga del
  // mundo, empezando por los ficheros MÁS PEQUEÑOS (cada uno cuesta una
  // suspensión igual que uno grande, así que los pequeños rinden más por MB).
  // Presupuesto en MB (cfg.warmMB) y lista en localStorage (atada a la clave de la caché).
  warmKey: 'vc_warm1',
  warmList: null,
  warmSeen: null,
  warmStarted: false,
  warmMaxFile: 12 * 1048576,
  noteWarm(canon, size) {
    if (!OD.cfg.warmMB || !canon) return;
    if (size && size > OD.warmMaxFile) return;
    if (/\.adf$/i.test(canon)) return;               // emisoras: van por su cuenta
    if (!OD.warmSeen) OD.warmSeen = new Set();
    if (OD.warmSeen.size < 20000) OD.warmSeen.add(canon);
  },
  warmLoad() {
    if (OD.warmList) return OD.warmList;
    let list = [];
    try {
      const raw = JSON.parse(localStorage.getItem(OD.warmKey) || 'null');
      if (raw && raw.tag === OD.cacheKey() && Array.isArray(raw.list)) list = raw.list;
    } catch (e) {}
    OD.warmList = list;
    return list;
  },
  warmPersist() {
    try {
      if (!OD.warmSeen || !OD.warmSeen.size) return;
      const set = new Set(OD.warmLoad());
      const n0 = set.size;
      for (const p of OD.warmSeen) set.add(p);
      if (set.size === n0) return;
      const list = [...set].slice(-6000);
      localStorage.setItem(OD.warmKey, JSON.stringify({ tag: OD.cacheKey(), list }));
      if (OD.verbose) odLog('[od] lista de arranque en caliente: ' + list.length + ' ficheros');
    } catch (e) {}
  },
  // El motor no avisa "voy a cargar el mundo"; sí llama a __loadProgress en
  // cada paso de CGame::InitialiseStep. Se engancha esa llamada: la primera
  // es el inicio de la carga, que es justo la ventana muerta donde adelantar
  // trabajo no cuesta al jugador (el cartel de carga ya está en pantalla).
  hookWorldLoad() {
    try {
      const w = typeof window !== 'undefined' ? window : null;
      if (!w || w.__odWarmHooked) return;
      w.__odWarmHooked = true;
      const wrap = () => {
        const orig = w.__loadProgress;
        if (orig && !orig.__odWrapped) {
          const f = function (a, b) {
            const now = Date.now();
            if (!OD._warmFired || now - (OD._warmLast || 0) > 8000) { OD._warmFired = 1; OD.warmStarted = false; setTimeout(() => OD.warmStart(), 250); }
            OD._warmLast = now;
            return orig.apply(this, arguments);
          };
          f.__odWrapped = true;
          w.__loadProgress = f;
          return true;
        }
        return false;
      };
      if (!wrap()) { let n = 0; const t = setInterval(() => { if (wrap() || ++n > 40) clearInterval(t); }, 250); }
    } catch (e) {}
  },
  warmStart() {
    if (OD.warmStarted || !OD.cfg.warmMB) return;
    OD.warmStarted = true;
    (async () => {
      try {
        let list = OD.warmLoad();
        if (!list.length) {
          const man0 = OD.manifest || {};
          list = Object.keys(man0).map((k) => '/' + man0[k].p).filter((p) => p && p.toLowerCase().indexOf('.adf') < 0);
          OD.trace('ODWARM sin lista previa: precarga amplia de ' + list.length + ' ficheros');
        }
        const t0 = Date.now();
        const budget = OD.cfg.warmMB * 1048576;
        const man = OD.manifest || {};
        const items = [];
        for (const p of list) {
          const h = man[OD.norm(p)];
          const s = (h && h.s) || 0;
          if (s && s <= OD.warmMaxFile) items.push([p, s]);
        }
        items.sort((a, b) => a[1] - b[1]);
        let n = 0, bytes = 0, touched = 0;
        for (const [p, s] of items) {
          if (bytes + s > budget) break;
          touched++;
          if (OD.exists(p)) continue;
          try { await OD._ensure(p); bytes += s; n++; } catch (e) {}
          if (n % 10 === 0) await OD.sleep(0);
        }
        OD.trace('ODWARM n=' + n + ' mb=' + Math.round(bytes / 1048576) + ' ms=' + (Date.now() - t0) +
          ' lista=' + items.length + ' ya_en_mem=' + (touched - n));
      } catch (e) { OD.trace('ODWARM error: ' + (e && e.message || e)); }
    })();
  },

  // ------------------------------------------------------------------------
  // Nivel 1 · El trabajo de carga en un Web Worker
  //
  // Antes, traer un fichero (leer la caché IDB y, si falta, red + escribir
  // caché) lo hacía el hilo del juego: el frame esperaba entero (medido, hipos
  // de 20-280 ms al entrar en zona nueva). Ahora ese trabajo va a un Worker y
  // al hilo del juego solo le queda copiar bytes a MEMFS (microsegundos).
  //
  // Además, si el juego ya está JUGANDO (lo publica el motor en
  // window.__vcInGame), un fichero que aún no está se contesta "no está" al
  // instante: el motor pide los modelos en cada pasada de streaming y reintenta,
  // así que ningún frame se congela y el fichero entra un par de frames después,
  // ya traído. Durante la CARGA (menú o partida) NO se aplaza nada: esa ruta es
  // crítica y exige los ficheros en el momento.
  // ------------------------------------------------------------------------
  wk: null,
  // R2: la emisora de 30 MB que se precarga retenida (sin escribir MEMFS) tiene
  // que caber aquí; antes eran 24 MB y se tiraba sola.
  wkReadyCap: 40 * 1048576,
  wkReady: null,
  wkReadyBytes: 0,
  wkPending: null,
  wkWaiters: null,
  wkFails: null,
  wkStats: null,
  wkErrs: null,
  wkErrShown: 0,
  wkMs: null,
  waitN: 0,
  waitMs: 0,
  wkLastLine: 0,
  _wkNote: null,
  wkInit() {
    OD.wkReady = new Map();
    OD.wkPending = {};
    OD.wkWaiters = {};
    OD.wkFails = {};
    OD.wkErrs = {};
    OD.wkStats = { ask: 0, take: 0, fast: 0, idb: 0, net: 0, err: 0, ms: 0, wrote: 0, held: 0 };
    OD.wkMs = {};
  },
  wkStart() {
    if (OD.wkWaiters && Object.keys(OD.wkWaiters).length) OD.wkFlushWaiters('reinicio-worker');
    OD.wkInit();
    if (OD.cfg.worker === false) { OD.wkNote('desactivado por configuracion'); OD.wk = null; return; }
    if (typeof Worker === 'undefined' || typeof Blob === 'undefined') { OD.wkNote('este navegador no tiene Worker'); return; }
    try {
      const url = URL.createObjectURL(new Blob([WORKER_SRC], { type: 'text/javascript' }));
      const w = new Worker(url);
      w.onmessage = (e) => OD.wkOnMsg(e);
      w.onerror = (e) => {
        OD.wk = null;
        // T17: un worker muerto dejaba a `wkWait` esperando para SIEMPRE (el
        // motor se quedaba suspendido y el juego congelado sin traza). Sus
        // esperas se cierran en el acto y el motor reintenta por el camino de
        // siempre en el siguiente frame.
        OD.wkFlushWaiters('worker-onerror');
        OD.wkNote('error del worker: ' + ((e && e.message) || e));
      };
      // OJO: un worker creado desde un Blob no tiene la página como base, así
      // que una ruta relativa NO se puede resolver ahí dentro ('Failed to parse
      // URL'). Se le manda la base ya absoluta.
      let absBase = OD.cfg.streamedUrl;
      try { absBase = new URL(OD.cfg.streamedUrl, location.href).href; } catch (e) {}
      if (absBase.slice(-1) !== '/') absBase += '/';
      w.postMessage({ op: 'init', streamedUrl: absBase });
      OD.wk = w;
    } catch (e) { OD.wk = null; OD.wkNote('no se pudo crear: ' + ((e && e.message) || e)); }
  },
  wkNote(msg) {
    if (OD._wkNote === msg) return;
    OD._wkNote = msg;
    odLog('[od] carga en hilo no disponible (' + msg + '): se usa el camino de siempre');
    OD.trace('ODWK off (' + msg + ')');
  },
  wkAsk(canon, hit) {
    if (!OD.wk || !OD.wkPending) return;
    if (OD.wkPending[canon]) return;
    OD.wkPending[canon] = 1;
    OD.wkStats.ask++;
    let rel = canon.replace(/^\//, '');
    let size = (hit && hit.s) || 0;
    try {
      const h = OD.manifest && OD.manifest[OD.norm(canon)];
      if (h) { if (h.p) rel = h.p; if (h.s) size = h.s; }
    } catch (e) {}
    try { OD.wk.postMessage({ op: 'get', key: canon, rel, size }); }
    catch (e) { delete OD.wkPending[canon]; OD.wkStats.err++; }
  },
  wkTake(canon) {
    if (!OD.wkReady) return null;
    const b = OD.wkReady.get(canon);
    if (!b) return null;
    OD.wkReady.delete(canon);
    OD.wkReadyBytes -= b.byteLength;
    return b;
  },
  wkPut(canon, buf) {
    if (!OD.wkReady) return;
    const prev = OD.wkReady.get(canon);
    if (prev) OD.wkReadyBytes -= prev.byteLength;
    OD.wkReady.set(canon, buf);
    OD.wkReadyBytes += buf.byteLength;
    while (OD.wkReadyBytes > OD.wkReadyCap) {
      const it = OD.wkReady.keys().next();
      if (it.done) break;
      OD.wkReadyBytes -= OD.wkReady.get(it.value).byteLength;
      OD.wkReady.delete(it.value);
    }
  },
  // T17: toda espera por el worker tiene fecha de caducidad. Antes, un mensaje
  // perdido o un worker muerto sin `onerror` dejaban el await del motor
  // pendiente para siempre: el juego se congelaba (mainSilentMs creciendo y
  // mark/ent quietos) sin error y sin forma de salir. Al vencer se contesta
  // null: el motor sigue y vuelve a pedirlo. `ms` largo solo en carga
  // bloqueante, donde el motor no puede reintentar en otro frame.
  wkWait(canon, ms) {
    const deadline = ms || 2500;
    return new Promise((res) => {
      if (!OD.wkWaiters) OD.wkInit();
      let done = false;
      const finish = (v) => { if (done) return; done = true; res(v); };
      const arr = (OD.wkWaiters[canon] = OD.wkWaiters[canon] || []);
      arr.push(finish);
      setTimeout(() => {
        if (done) return;
        const a = OD.wkWaiters[canon];
        if (a) {
          const i = a.indexOf(finish);
          if (i >= 0) a.splice(i, 1);
          if (!a.length) { delete OD.wkWaiters[canon]; delete OD.wkPending[canon]; }
        }
        OD.wkTimeouts++;
        if (OD.wkTimeouts <= 8) OD.trace('ODWKTMO ' + canon + ' ms=' + deadline);
        finish(null);
      }, deadline);
    });
  },
  wkTimeouts: 0,
  wkFlushWaiters(why) {
    const w = OD.wkWaiters || {};
    OD.wkWaiters = {};
    OD.wkPending = {};
    let n = 0;
    for (const k of Object.keys(w)) {
      const arr = w[k] || [];
      for (const fn of arr) { n++; try { fn(null); } catch (e) {} }
    }
    if (n) OD.trace('ODWKDEAD ' + why + ' esperas=' + n);
  },
  // Solo se aplaza en partida, y nunca encadenando fallos del mismo fichero
  // (tras 8 seguidos se espera: antes esperar que quedarse sin fichero).
  wkFastFail(canon) {
    if (!OD.wk) return false;
    if (typeof window === 'undefined' || window.__vcInGame !== true) return false;
    // El motor avisa cuando está en una carga BLOQUEANTE (LoadAllRequestedModels,
    // preparación de cinemática): ahí no se aplaza nada, porque el motor no
    // puede reintentar en otro frame y se quedaría sin el modelo/audio.
    if ((window.__vcODBlock | 0) > 0) return false;
    // Tras 3 aplazamientos seguidos del MISMO fichero se espera: durante una
    // carga bloqueante (escena de cinemática, preload de audio) el motor no
    // puede reintentar en otro frame y se quedaría sin el fichero. Esperar 3
    // frames (~50 ms) antes de darse por vencido es barato y evita escenas
    // vacías.
    if ((OD.wkFails[canon] || 0) >= 3) return false;
    // Un fichero que el worker no puede traer (no existe, 404) no se aplaza
    // para siempre: tras 3 fallos se deja pasar al camino de siempre, que ya
    // sabe fallar de forma controlada.
    if ((OD.wkErrs[canon] || 0) >= 3) return false;
    OD.wkFails[canon] = (OD.wkFails[canon] || 0) + 1;
    OD.wkStats.fast++;
    return true;
  },
  // true si el worker está vivo y el juego en partida (para no caer al camino
  // bloqueante cuando el worker falla: en partida se reintenta en otro frame).
  wkInGame() {
    return !!OD.wk && typeof window !== 'undefined' && window.__vcInGame === true;
  },
  wkOnMsg(e) {
    const m = (e && e.data) || {};
    if (m.op === 'ready') { if (OD.verbose) odLog('[od] worker de carga listo'); return; }
    if (!OD.wkPending) OD.wkInit();
    delete OD.wkPending[m.key];
    delete OD.wkFails[m.key];
    if (m.op === 'err') {
      // Los primeros fallos, siempre en el log (con esto se ve la causa real
      // sin adivinar); después basta con el contador.
      OD.wkErrs[m.key] = (OD.wkErrs[m.key] || 0) + 1;
      if (OD.wkErrShown < 5) {
        OD.wkErrShown++;
        OD.trace('ODWKERR ' + m.key + ' :: ' + m.msg + ' (' + (m.ms || 0) + 'ms)');
      }
    }
    if (m.op === 'data') {
      OD.wkStats[m.src === 'idb' ? 'idb' : 'net']++;
      OD.wkStats.ms += m.ms || 0;
      OD.wkMs[m.key] = [m.ms || 0, m.src === 'idb' ? 'idb' : 'red'];
      const arr = OD.wkWaiters[m.key];
      if (arr) { delete OD.wkWaiters[m.key]; for (const fn of arr) fn(m.buf); }
      else {
        // Nadie espera (lo normal en partida: el frame siguió sin el fichero).
        // Se escribe en MEMFS YA — copiar es microscópico — para que el motor
        // lo encuentre puesto cuando lo vuelva a pedir, en vez de quedarse en
        // memoria esperando una petición que podría no llegar (huecos).
        try { OD.writeFile(m.key, m.buf); } catch (e) { OD.wkPut(m.key, m.buf); }
      }
      OD.wkStats.take++;
      OD.wkMaybeTrace();
      return;
    }
    if (m.op === 'err') {
      OD.wkStats.err++;
      const arr = OD.wkWaiters[m.key];
      if (arr) { delete OD.wkWaiters[m.key]; for (const fn of arr) fn(null); }
      if (OD.verbose) odLog('[od] worker falló ' + m.key + ': ' + m.msg);
      OD.wkMaybeTrace();
    }
  },
  wkMaybeTrace() {
    const now = Date.now();
    if (now - OD.wkLastLine < 30000) return;
    OD.wkLastLine = now;
    const s = OD.wkStats;
    OD.trace('ODWK peticiones=' + s.ask + ' aplazados=' + s.fast +
      ' entregados=' + s.take + ' escritos=' + s.wrote + ' esperando=' + Math.round(OD.wkReadyBytes / 1048576) + 'MB' +
      ' idb=' + s.idb + ' red=' + s.net + ' err=' + s.err + ' ms=' + Math.round(s.ms) +
      ' esperas100=' + OD.waitN + ' ms_esp=' + Math.round(OD.waitMs));
  },
  // --------------------------------------------------------------------
  // R2 (plan 06, 5ª partida): diagnóstico del congelado al cargar emisora.
  //
  // En la traza del jugador el final de la partida es un tirón de **1256 ms de
  // espera** (`FPHASE wait=1256.5`, con la lógica a 0,3 ms): un bloqueo del hilo
  // del juego. Desde esta capa sólo hay dos cosas capaces de hacerlo:
  //   1. que `ensure` ESPERE (await) por un `.adf` de emisora; y
  //   2. que la entrega escriba un fichero grande en MEMFS en el hilo principal
  //      (la copia no se puede evitar: el motor lo lee con fopen sobre MEMFS).
  // `ODSTA` deja constancia de lo primero y `ODWRITE` mide lo segundo. Sin estos
  // dos datos, "la emisora congela" es teoría.
  // --------------------------------------------------------------------
  _adfLog: null,
  adfTrace(name, memfs, ms, extra) {
    let now;
    try { now = performance.now(); } catch (e) { now = Date.now(); }
    if (!OD._adfLog) OD._adfLog = {};
    const st = OD._adfLog[name] || (OD._adfLog[name] = { t: -1e9, n: 0 });
    st.n++;
    if (now - st.t < 1000) return;          // una línea por segundo y emisora
    st.t = now;
    let block = 0;
    try { block = window.__vcODBlock | 0; } catch (e) {}
    OD.trace('ODSTA ' + name + ' memfs=' + (memfs ? 1 : 0) + ' n=' + st.n
      + ' ms=' + Math.round(ms || 0) + ' block=' + block + (extra ? ' ' + extra : ''));
  },

  // Escritura a MEMFS: lo ÚNICO que le queda al hilo del juego (una copia).
  writeFile(canon, buffer) {
    let t0 = 0;
    try { t0 = performance.now(); } catch (e) {}
    const u8 = new Uint8Array(buffer);
    const slash = canon.lastIndexOf('/');
    const parent = slash > 0 ? canon.slice(0, slash) : '/';
    const name = canon.slice(slash + 1);
    try { FS.mkdirTree(parent); } catch (e) {}
    try { FS.unlink(canon); } catch (e) {}
    FS.createDataFile(parent, name, u8, true, true, true);
    if (OD.wkStats) OD.wkStats.wrote++;
    OD.touch(canon, u8.length);
    OD.idbTouch(canon);
    OD.noteWarm(canon, u8.length);
    // R2: sólo los grandes (los que pueden notarse como tirón).
    if (u8.length > 4 * 1048576) {
      let ms = 0;
      try { ms = performance.now() - t0; } catch (e) { ms = 0; }
      OD.trace('ODWRITE ' + canon + ' mb=' + (u8.length / 1048576).toFixed(1) + ' ms=' + Math.round(ms));
    }
    return canon;
  },

  idbGet(key) {
    return new Promise((res) => {
      if (!OD.db) return res(null);
      try {
        const t = OD.db.transaction(['files'], 'readonly').objectStore('files').get(key);
        t.onsuccess = (e) => res(e.target.result || null);
        t.onerror = () => res(null);
      } catch (e) { res(null); }
    });
  },

  idbPut(key, buf) {
    return new Promise((res) => {
      if (!OD.db) return res();
      try {
        const t = OD.db.transaction(['files'], 'readwrite').objectStore('files').put(buf, key);
        t.onsuccess = () => res();
        t.onerror = () => res();
      } catch (e) { res(); }
    });
  },

  idbClear() {
    return new Promise((res) => {
      if (!OD.db) return res();
      try {
        const tx = OD.db.transaction(['files', 'meta'], 'readwrite');
        tx.objectStore('files').clear();
        tx.objectStore('meta').clear();
        tx.oncomplete = () => res();
        tx.onerror = () => res();
      } catch (e) { res(); }
    });
  },

  // ------------------------------------------------------------------------
  // F6a · Caché IDB con techo y desalojo por uso (LRU)
  //
  // 'vcod2' guarda una copia de TODO lo que se ha traído nunca: es lo que hace
  // que el primer minuto no dependa de la red, pero crecía sin cota (medido:
  // 1.280 MB en una sesión larga). Ahora hay tope (`idbCapMB`) y cuando se pasa
  // se borra lo MÁS VIEJO en uso hasta quedar por debajo; lo borrado se
  // re-descarga solo si se vuelve a necesitar. El uso se anota en el store
  // 'meta' ({t, s} por clave) en lotes, para no escribir en cada lectura.
  // ------------------------------------------------------------------------
  _idbPairs(store, keysOnly) {
    return new Promise((res) => {
      if (!OD.db) return res([]);
      try {
        const tx = OD.db.transaction([store], 'readonly');
        const st = tx.objectStore(store);
        const kq = st.getAllKeys();
        const vq = keysOnly ? null : st.getAll();
        tx.oncomplete = () => {
          const ks = kq.result || [];
          if (keysOnly) { res(ks.map((k) => [k, null])); return; }
          const vs = vq.result || [];
          res(ks.map((k, i) => [k, vs[i]]));
        };
        tx.onerror = () => res([]);
      } catch (e) { res([]); }
    });
  },

  idbMetaPut(entries) {           // entries: [[clave, {t,s}], ...] en UNA transacción
    return new Promise((res) => {
      if (!OD.db || !entries.length) return res();
      try {
        const tx = OD.db.transaction(['meta'], 'readwrite');
        const st = tx.objectStore('meta');
        for (const [k, v] of entries) st.put(v, k);
        tx.oncomplete = () => res();
        tx.onerror = () => res();
      } catch (e) { res(); }
    });
  },

  idbDelete(keys) {
    return new Promise((res) => {
      if (!OD.db || !keys.length) return res();
      try {
        const tx = OD.db.transaction(['files'], 'readwrite');
        const st = tx.objectStore('files');
        for (const k of keys) st.delete(k);
        tx.oncomplete = () => res();
        tx.onerror = () => res();
      } catch (e) { res(); }
    });
  },

  // Acceso (lectura de caché o descarga): se anota el uso en lote.
  _idbTouch: null,
  idbTouch(key) {
    if (!OD.db) return;
    if (!OD._idbTouch) OD._idbTouch = new Set();
    if (OD._idbTouch.size < 8000) OD._idbTouch.add(key);
  },

  // Tamaño de un fichero de la caché: de 'meta' si lo tiene, si no del
  // manifiesto (las entradas escritas antes de F6a no tienen 'meta').
  idbSizeOf(key, meta) {
    if (meta && meta.s) return meta.s;
    try { const h = OD.manifest && OD.manifest[OD.norm(key)]; if (h && h.s) return h.s; } catch (e) {}
    return 0;
  },

  async idbTrim(reason) {
    if (!OD.db) return;
    const capMB = OD.cfg.idbCapMB;
    if (!capMB || capMB <= 0) return;
    const t0 = Date.now();
    const pairs = (await OD._idbPairs('files', true)).filter(([k]) => k !== OD.cacheKeyName);
    if (!pairs.length) return;
    const metas = new Map(await OD._idbPairs('meta'));
    let total = 0;
    const ents = [];
    for (const [k] of pairs) {
      const m = metas.get(k) || null;
      const s = OD.idbSizeOf(k, m);
      total += s;
      ents.push([k, (m && m.t) || 0, s]);
    }
    const totalMB = Math.round(total / 1048576);
    if (total <= capMB * 1048576) {
      OD.trace('IDBTRIM motivo=' + (reason || 'init') + ' n=' + pairs.length + ' uso=' + totalMB + 'MB tope=' + capMB + 'MB (nada que borrar)');
      return;
    }
    ents.sort((a, b) => a[1] - b[1]);              // más viejo (o sin marca) primero
    OD.idbFlushTouches();                            // que lo accedido ahora no se borre
    const target = capMB * 1048576 * 0.9;            // margen: no repetir el trim cada sesión
    const drop = [];
    let freed = 0;
    for (const [k, , s] of ents) {
      if (total - freed <= target) break;
      drop.push(k);
      freed += s;
    }
    await OD.idbDelete(drop);
    OD.trace('IDBTRIM motivo=' + (reason || 'init') + ' antes=' + totalMB + 'MB despues=' +
      Math.round((total - freed) / 1048576) + 'MB borrados=' + drop.length + ' tope=' + capMB +
      'MB ms=' + (Date.now() - t0));
    odLog('[od] caché de datos: ' + totalMB + 'MB -> ' + Math.round((total - freed) / 1048576) +
      'MB (borrados ' + drop.length + ' ficheros, tope ' + capMB + 'MB)');
    OD.trimmed = drop.length;
  },

  async idbFlushTouches() {
    if (!OD._idbTouch || !OD._idbTouch.size) return;
    const keys = [...OD._idbTouch];
    OD._idbTouch = new Set();
    const now = Date.now();
    await OD.idbMetaPut(keys.map((k) => [k, { t: now, s: OD.idbSizeOf(k, null) }]));
  },

  // Purga por versión de datos (ver el sello __vc__ del manifiesto). Imprescindible
  // al regenerar assets: sin esto el navegador sigue sirviendo los bytes viejos.
  cacheKeyName: '__cachekey__',
  cacheKey() {
    try {
      const h = OD.manifest && OD.manifest['__vc__'] && OD.manifest['__vc__'].h;
      if (h) return 'm' + h;
    } catch (e) {}
    return OD.dataTag;
  },
  async idbPurgeIfStale() {
    if (!OD.db) return;
    const key = OD.cacheKey();
    const prev = await OD.idbGet(OD.cacheKeyName);
    if (prev !== key) {
      await OD.idbClear();
      await OD.idbPut(OD.cacheKeyName, key);
      odLog('[od] caché de datos purgada (' + (prev || 'vacía') + ' -> ' + key + ')');
      OD.trace('DATATAG old=' + (prev || 'ninguno') + ' new=' + key + ' (caché IDB purgada)');
    }
  },

  idbGetT(key) {
    // IDB no debe colgar nunca el loop: a los 8 s se trata como fallo de caché.
    return Promise.race([
      OD.idbGet(key),
      new Promise((res) => setTimeout(() => res(null), 8000)),
    ]);
  },

  // Descarga de red con timeout de inactividad + aborto (F1: un fetch colgado
  // congelaba el loop Asyncify en silencio para siempre). Lanza Error si falla.
  async dlNetwork(canon, hit, attempt) {
    const slash = canon.lastIndexOf('/');
    const parent = slash > 0 ? canon.slice(0, slash) : '/';
    const name = canon.slice(slash + 1);
    const tmp = canon + '.part';
    const INACT_MS = 25000;
    const ctrl = new AbortController();
    let timer = null;
    const poke = () => {
      if (timer) clearTimeout(timer);
      timer = setTimeout(() => { try { ctrl.abort(); } catch (e) {} }, INACT_MS);
    };
    const pend = OD.pending[canon];
    const isHtmlHead = (u8) => {
      if (u8.length < 9) return false;
      const h = String.fromCharCode(...u8.slice(0, 9)).toLowerCase();
      return h.startsWith('<!doctype') || h.startsWith('<html');
    };
    try { FS.mkdirTree(parent); } catch (e) {}
    try { FS.unlink(tmp); } catch (e) {}
    poke();
    let r;
    try {
      r = await fetch(OD.cfg.streamedUrl + hit.p.split('/').map(encodeURIComponent).join('/'), { signal: ctrl.signal });
    } catch (e) {
      throw new Error('fetch intento ' + attempt + ' (timeout ' + (INACT_MS / 1000) + 's?): ' + (e && e.message || e));
    }
    if (!r.ok) throw new Error('HTTP ' + r.status);
    const ct = (r.headers.get('content-type') || '').toLowerCase();
    if (ct.includes('text/html')) throw new Error('HTML en vez de fichero');
    const total = Number(r.headers.get('Content-Length')) || hit.s || 0;
    if (!r.body || !r.body.getReader) throw new Error('sin body legible');
    const reader = r.body.getReader();
    let off = 0, first = true, idbBuf = (hit.s <= 8 * 1048576) ? [] : null, st = null;
    const ensureOpen = () => {
      if (!st) {
        if (total > 0) FS.createDataFile(parent, name + '.part', new Uint8Array(total), true, true, true);
        else FS.createDataFile(parent, name + '.part', new Uint8Array(0), true, true, true);
        st = FS.open(tmp, 'w');
      }
    };
    try {
      for (;;) {
        let rd;
        try { rd = await reader.read(); }
        catch (e) { throw new Error('red cortada en ' + off + 'b: ' + (e && e.message || e)); }
        const { done, value } = rd;
        if (done) break;
        if (first) {
          first = false;
          if (isHtmlHead(value)) throw new Error('HTML en vez de fichero');
        }
        ensureOpen();
        FS.write(st, value, 0, value.length, off);
        off += value.length;
        if (pend) { pend.last = Date.now(); pend.bytes = off; }
        if (idbBuf) idbBuf.push(value);
        poke();
        await OD.sleep(0);
      }
    } finally {
      try { reader.cancel(); } catch (e) {}
    }
    if (timer) clearTimeout(timer);
    if (st) FS.close(st);
    if (!st || off === 0) { try { FS.unlink(tmp); } catch (e) {} throw new Error('fichero vacío'); }
    if (total > 0 && off < total) {
      try { FS.unlink(tmp); } catch (e) {}
      throw new Error('truncado ' + off + '/' + total);
    }
    try { FS.rename(tmp, canon); } catch (e) { throw new Error('rename: ' + e); }
    OD.touch(canon, off);
    OD.fetchedBytes += off;
    OD.fetchedCount++;
    if (idbBuf) {
      const flat = new Uint8Array(off);
      let p = 0;
      for (const c of idbBuf) { flat.set(c, p); p += c.length; }
      idbBuf = null;
      await OD.idbPut(canon, flat.buffer);
      await OD.idbMetaPut([[canon, { t: Date.now(), s: off }]]);
      OD.noteWarm(canon, off);
    }
    if (OD.verbose && off > 32 * 1048576) odLog('[od] OK grande ' + canon);
  },
  exists(path) {
    try { FS.stat(path); return true; } catch (e) { return false; }
  },

  // Devuelve ruta canónica lista para abrir, o null.
  // F4a: coste de las suspensiones que pide el wasm (wall ms de ensure).
  // F5c: aquí también se detecta la emisora que pide el motor (todas las
  // vías pasan por aquí; las prefetch internas usan _ensure y no disparan).
  // F5d: fast-fail de radios — si el .adf no está aún en MEMFS, NO suspender
  // trayéndolo (frame congelado de 170-250 ms medido en F5c): fallar al
  // instante (radio muda 1-2 s), traerlo en background y dejar que el motor
  // re-intente el open (MusicManager reintenta mientras !IsStreamPlaying).
  asyncN: 0, asyncMs: 0,
  async ensure(reqPath) {
    const t0 = (typeof performance !== 'undefined') ? performance.now() : Date.now();
    try {
      const s = String(reqPath).replace(/\\/g, '/');
      // (el arranque en caliente se engancha en init(): hookWorldLoad)
      try {
        const sm = /([a-z0-9_]+)\.adf$/i.exec(s);
        if (sm && OD.radios.includes(sm[1].toUpperCase())) {
          const station = sm[1].toUpperCase();
          OD.noteStation(station);
          const hit = OD.manifest && OD.manifest[OD.norm(s)];
          const canon = hit ? '/' + hit.p : ('/Audio/' + station + '.ADF');
          let inMem = false;
          try { FS.stat(canon); inMem = true; } catch (e) {}
          if (inMem) {
            OD.adfTrace(station, 1, 0, '');
          } else {
            // R2: NUNCA se espera (await) por una emisora, ni siquiera durante
            // una carga bloqueante del motor: ahí es donde está el tirón de
            // 1256 ms medido. El motor reintenta mientras !IsStreamPlaying, así
            // que el peor caso es radio muda 1-2 s, como en el original. Los
            // bytes los trae el worker, fuera del hilo del juego.
            if (OD.wk) OD.wkAsk('/Audio/' + station + '.ADF', hit);
            else OD._ensure('/Audio/' + station + '.ADF').catch(() => {});
            OD.adfTrace(station, 0, 0, 'espera=0' + (OD.wk ? ' wk=1' : ' wk=0'));
            return null;
          }
        }
      } catch (e) {}
      return await OD._ensure(reqPath);
    }
    finally {
      OD.asyncN++;
      OD.asyncMs += ((typeof performance !== 'undefined') ? performance.now() : Date.now()) - t0;
    }
  },
  noFetchConteo: {},
  noFetchMotivos: {},
  noFetchCierre: 0,
  noFetch(reqPath, canon, why, t0) {
    // T38 (49.2): el instrumento del 49.1 se desboco. En la partida del vuelo
    // dejo 936 lineas y en la siguiente 1664, todas del mismo motivo, y eso es
    // la basura que el jugador pidio borrar. La primera vez de cada motivo se
    // emite entera (es la que nombra el caso) y el resto se agrega en un
    // resumen cada 5 s con el fichero que mas se repite.
    const clave = (canon || '-') + '|' + why;
    const now = Date.now();
    OD.noFetchConteo[clave] = (OD.noFetchConteo[clave] || 0) + 1;
    try {
      if (!OD.noFetchMotivos[why]) {
        OD.noFetchMotivos[why] = 1;
        OD.trace('ODNOFETCH primero porque=' + why + ' path=' + reqPath +
          ' canon=' + (canon || '-') + ' ms=' + (now - t0));
      }
      if (now >= OD.noFetchCierre) {
        OD.noFetchCierre = now + 5000;
        const claves = Object.keys(OD.noFetchConteo);
        if (claves.length === 0) return;
        let total = 0, pico = '', picoN = 0;
        for (const c of claves) {
          total += OD.noFetchConteo[c];
          if (OD.noFetchConteo[c] > picoN) { picoN = OD.noFetchConteo[c]; pico = c; }
        }
        OD.trace('ODNOFETCH n=' + total + ' unicos=' + claves.length + ' pico=' + picoN +
          ' porque=' + (pico.split('|')[1] || '-') + ' fichero=' + (pico.split('|')[0] || '-'));
        OD.noFetchConteo = {};
      }
    } catch (e) {}
  },
  presentSync(reqPath) {
    let cwd = '/';
    try { cwd = FS.cwd(); } catch (e) {}
    const cands = [OD.norm(reqPath), OD.norm(cwd + '/' + reqPath)];
    for (const n of cands) {
      const f = '/' + n;
      try { FS.stat(f); if (OD.live.has(f)) OD.touch(f, 0); return f; } catch (e) {}
    }
    let canon = null;
    for (const n of cands) {
      if (OD.manifest && OD.manifest[n]) { canon = '/' + OD.manifest[n].p; break; }
    }
    if (canon && OD.exists(canon)) return canon;
    return null;
  },
  async _ensure(reqPath) {
    // El motor abre a veces con ruta relativa al CWD (p. ej. "SPANISH.GXT"
    // con CWD=/TEXT). Probar tal cual + contra CWD.
    const t0 = Date.now();
    const present = OD.presentSync(reqPath);
    if (present) return present;
    let cwd = '/';
    try { cwd = FS.cwd(); } catch (e) {}
    const cands = [OD.norm(reqPath), OD.norm(cwd + '/' + reqPath)];
    let hit = null, canon = null;
    for (const n of cands) {
      if (OD.manifest && OD.manifest[n]) { hit = OD.manifest[n]; canon = '/' + hit.p; break; }
    }
    if (!hit) {
      // 21/10: el motor pide un fichero que el manifest no tiene -> el
      // motor reintenta en bucle y la carga no acaba. Hay que verlo.
      try { OD.trace('ODMISS fuera-de-manifest ' + reqPath); } catch (e) {}
      return null;
    }

    // Camino nuevo (Nivel 1): el worker trae el fichero fuera del hilo del
    // juego. Si ya lo tiene entregado, esto es solo copiarlo a MEMFS. Si no,
    // se pide y: en partida se aplaza (el motor reintenta sin congelar el
    // frame) o, durante la carga, se espera como antes. Si el worker falla, se
    // sigue por el camino de siempre (debajo), que se queda como red de
    // seguridad.
    if (OD.wk) {
      const ready = OD.wkTake(canon);
      if (ready) return OD.writeFile(canon, ready);
      OD.wkAsk(canon, hit);
      if (OD.wkFastFail(canon)) {
        OD.noFetch(reqPath, canon, 'presupuesto', t0);
        return null;
      }
      // T18: en partida la espera se acota a 2 s (antes 30 s dentro de una carga
      // bloqueante, que es donde se quedó congelado el juego al salir un cop del
      // coche y pedir su .ifp). Cargando (init, escena desde el menú) se mantiene
      // larga: ahí sí es la ruta crítica.
      const inGame = (typeof window !== 'undefined' && window.__vcInGame === true);
      const wt0 = Date.now();
      const delivered = await OD.wkWait(canon, inGame ? 2000 : 30000);
      const wwait = Date.now() - wt0;
      let wpath = null, wmem = 0;
      if (delivered) {
        const mt0 = Date.now();
        wpath = OD.writeFile(canon, delivered);
        wmem = Date.now() - mt0;
      }
      if (wwait > 100) {
        const wi = OD.wkMs[canon] || [-1, '-'];
        OD.waitN++; OD.waitMs += wwait;
        OD.trace('ODWAIT ' + canon + ' espera=' + wwait + ' worker=' + wi[0] + ' memfs=' + wmem + ' src=' + wi[1]);
      }
      delete OD.wkMs[canon];
      if (delivered) return wpath;
      // El worker no pudo traerlo: en partida se reintenta en otro frame (el
      // motor lo vuelve a pedir); durante la carga se sigue por el camino de
      // siempre, que es la ruta crítica y no admite aplazamientos.
      if (OD.wkInGame()) {
        OD.noFetch(reqPath, canon, 'worker', t0);
        return null;
      }
    }

    if (OD.inflight[canon]) { await OD.inflight[canon]; return OD.exists(canon) ? canon : null; }
    const job = (async () => {
      OD.misses.push(canon + ' (' + hit.s + ')');
      if (OD.verbose && OD.misses.length % 50 === 1) odLog('[od] +' + canon);
      // Higiene de memoria: reintentar IDB (rápido, sin red) y solo bajar lo
      // que falte; escribir por tramos cediendo; IDB con contrapresión y tope.
      let cached = await OD.idbGetT(canon);
      if (cached) OD.idbHits++;
      const slash = canon.lastIndexOf('/');
      const parent = slash > 0 ? canon.slice(0, slash) : '/';
      const name = canon.slice(slash + 1);
      try { FS.mkdirTree(parent); } catch (e) {}
      try { FS.unlink(canon); } catch (e) {}
      const CH = 2 * 1048576;
      const isHtmlHead = (u8) => {
        if (u8.length < 9) return false;
        const h = String.fromCharCode(...u8.slice(0, 9)).toLowerCase();
        return h.startsWith('<!doctype') || h.startsWith('<html');
      };
      if (cached && cached.byteLength && (!hit.s || cached.byteLength >= hit.s)) {
        const u8 = new Uint8Array(cached);
        if (isHtmlHead(u8)) throw new Error('caché envenenada, re-descargar');
        FS.createDataFile(parent, name, new Uint8Array(0), true, true, true);
        const st = FS.open(canon, 'w');
        for (let off = 0; off < u8.length; off += CH) {
          FS.write(st, u8, off, Math.min(CH, u8.length - off), off);
          if (off + CH < u8.length) await new Promise((r) => setTimeout(r, 0));
        }
        FS.close(st);
        OD.touch(canon, u8.length);
        OD.idbTouch(canon);            // marca de uso (lote) para el techo LRU
        OD.noteWarm(canon, u8.length); // candidato al arranque en caliente
        return;
      }
      // Red con reintentos (F1): si un intento se cuelga o falla, se reintenta;
      // tras 3 fallos se devuelve null (fallo suave, como un 404).
      OD.pending[canon] = { t0: Date.now(), last: Date.now(), bytes: 0, try: 0 };
      await OD.dlSlot();
      try {
        let lastErr = null;
        for (let attempt = 1; attempt <= 3; attempt++) {
          OD.pending[canon].try = attempt;
          OD.pending[canon].last = Date.now();
          try {
            await OD.dlNetwork(canon, hit, attempt);
            lastErr = null;
            break;
          } catch (e) {
            lastErr = e;
            odLog('[od] REINTENTO ' + attempt + '/3 ' + canon + ': ' + (e && e.message || e));
            try { FS.unlink(canon + '.part'); } catch (ee) {}
            await OD.sleep(1000 * attempt);
          }
        }
        if (lastErr) throw lastErr;
      } finally {
        delete OD.pending[canon];
        OD.dlRelease();
      }
    })();
    OD.inflight[canon] = job;
    try { await job;     } catch (e) {
      odLog('[od] FAIL ' + canon + ': ' + (e && e.message || e));
      // No dejar parciales: un .part a medias cuenta en el censo y confunde.
      try { FS.unlink(canon + '.part'); } catch (ee) {}
    }
    delete OD.inflight[canon];
    if (OD.exists(canon)) return canon;
    OD.noFetch(reqPath, canon, 'descarga', t0);
    return null;
  },
};

window.__vcRaf = { n: 0 };
(function vcRafCount() {
  window.__vcRaf.n++;
  try { requestAnimationFrame(vcRafCount); } catch (e) {}
})();

if (typeof Module !== 'undefined') {
  if (!Module.preRun) Module.preRun = [];
  Module.preRun.push(() => {
    addRunDependency('od-init');
    OD.init().then(() => removeRunDependency('od-init')).catch(() => removeRunDependency('od-init'));
  });
  // Tripwire temporal: registra quién crea/extiende ficheros grandes.
  Module.postRun = Module.postRun || [];
  Module.postRun.push(() => {
    const stk = () => {
      try { return new Error().stack.split('\n').slice(2, 6).join(' <- ').slice(0, 300); }
      catch (e) { return '?'; }
    };
    try {
      const origCreate = FS.createDataFile;
      FS.createDataFile = function (parent, name, data, canRead, canWrite, canOwn) {
        try {
          const sz = data ? data.length : 0;
          if (sz > 50 * 1048576) odLog('[od] BIGCREATE ' + parent + '/' + name + ' ' + Math.round(sz / 1048576) + 'MB :: ' + stk());
        } catch (e) {}
        return origCreate.call(FS, parent, name, data, canRead, canWrite, canOwn);
      };
      const origWrite = FS.write;
      FS.write = function (stream, buffer, offset, length, position) {
        try {
          if (length > 50 * 1048576 || position > 100 * 1048576) {
            odLog('[od] BIGWRITE ' + (stream.path || stream.nodeName || '?') + ' pos=' + position + ' len=' + length + ' :: ' + stk());
          }
        } catch (e) {}
        try {
          const p = stream.path || stream.nodeName || '';
          if (p.indexOf('/userfiles') === 0) OD.noteUserfilesWrite(p);
        } catch (e) {}
        return origWrite.call(FS, stream, buffer, offset, length, position);
      };
      const origTrunc = FS.truncate;
      FS.truncate = function (path, len) {
        try {
          if (len > 100 * 1048576) odLog('[od] BIGTRUNC ' + path + ' ' + len + ' :: ' + stk());
        } catch (e) {}
        return origTrunc.call(FS, path, len);
      };
      const origFtrunc = FS.ftruncate;
      FS.ftruncate = function (fd, len) {
        try {
          if (len > 100 * 1048576) {
            let p = '?';
            try { p = FS.getStreamChecked(fd).path; } catch (e) {}
            odLog('[od] BIGFTRUNC fd=' + fd + ' path=' + p + ' len=' + len + ' :: ' + stk());
          }
        } catch (e) {}
        return origFtrunc.call(FS, fd, len);
      };
      try {
        const paths = ['/models/txd.img'];
        for (const p of paths) {
          try {
            const st = FS.stat(p);
            odLog('[od] ARRANQUE existe ' + p + ' size=' + st.size + ' mode=' + st.mode);
          } catch (e) { odLog('[od] ARRANQUE no existe ' + p); }
        }
      } catch (e) {}
      try {
        if (FS.mmap) {
          const origMmap = FS.mmap;
          FS.mmap = function (stream, length, position, prot, flags) {
            try {
              if (length > 100 * 1048576) odLog('[od] BIGMMAP ' + (stream.path || '?') + ' len=' + length + ' :: ' + stk());
            } catch (e) {}
            return origMmap.call(FS, stream, length, position, prot, flags);
          };
        }
      } catch (e) {}
      // Watchdog F1: vive en el event loop del navegador, así que sigue
      // hablando aunque el loop wasm quede suspendido en un await eterno.
      try {
        setInterval(() => {
          try {
            const now = Date.now();
            for (const k of Object.keys(OD.pending || {})) {
              const p = OD.pending[k];
              const age = Math.round((now - p.t0) / 1000);
              const idle = Math.round((now - p.last) / 1000);
              if (age >= 20) odLog('[od] STALL ' + k + ' edad=' + age + 's inactivo=' + idle + 's bytes=' + p.bytes + ' intento=' + p.try);
            }
          } catch (e) {}
        }, 3000);
      } catch (e) {}
      const origRename = FS.rename;      FS.rename = function (a, b) {
        try {
          let sz = -1;
          try { sz = FS.stat(b).size; } catch (e) {}
          if (sz > 50 * 1048576) odLog('[od] BIGRENAME ' + a + ' -> ' + b + ' dest ' + Math.round(sz / 1048576) + 'MB :: ' + stk());
        } catch (e) {}
        return origRename.call(FS, a, b);
      };
    } catch (e) {}
  });
}
// end include: C:/Users/s0rno/OneDrive/Documents/lance-vance/gta_vc_browser/web/ondemand.js


var programArgs = [];
var thisProgram = './this.program';
var quit_ = (status, toThrow) => {
  throw toThrow;
};

// In MODULARIZE mode _scriptName needs to be captured already at the very top of the page immediately when the page is parsed, so it is generated there
// before the page load. In non-MODULARIZE modes generate it here.
var _scriptName = globalThis.document?.currentScript?.src;

if (typeof __filename != 'undefined') { // Node
  _scriptName = __filename;
} else
if (ENVIRONMENT_IS_WORKER) {
  _scriptName = self.location.href;
}

// `/` should be present at the end if `scriptDirectory` is not empty
var scriptDirectory = '';
function locateFile(path) {
  if (Module['locateFile']) {
    return Module['locateFile'](path, scriptDirectory);
  }
  return scriptDirectory + path;
}

// Hooks that are implemented differently in different runtime environments.
var readAsync, readBinary;

if (ENVIRONMENT_IS_NODE) {

  // These modules will usually be used on Node.js. Load them eagerly to avoid
  // the complexity of lazy-loading.
  var fs = require('node:fs');

  scriptDirectory = __dirname + '/';

// include: node_shell_read.js
readBinary = (filename) => {
  // We need to re-wrap `file://` strings to URLs.
  filename = isFileURI(filename) ? new URL(filename) : filename;
  var ret = fs.readFileSync(filename);
  return ret;
};

readAsync = async (filename, binary = true) => {
  // See the comment in the `readBinary` function.
  filename = isFileURI(filename) ? new URL(filename) : filename;
  var ret = fs.readFileSync(filename, binary ? undefined : 'utf8');
  return ret;
};
// end include: node_shell_read.js
  if (process.argv.length > 1) {
    thisProgram = process.argv[1].replace(/\\/g, '/');
  }

  programArgs = process.argv.slice(2);

  // MODULARIZE will export the module in the proper place outside, we don't need to export here
  if (typeof module != 'undefined') {
    module['exports'] = Module;
  }

  quit_ = (status, toThrow) => {
    process.exitCode = status;
    throw toThrow;
  };

} else

// Note that this includes Node.js workers when relevant (pthreads is enabled).
// Node.js workers are detected as a combination of ENVIRONMENT_IS_WORKER and
// ENVIRONMENT_IS_NODE.
if (ENVIRONMENT_IS_WEB || ENVIRONMENT_IS_WORKER) {
  try {
    scriptDirectory = new URL('.', _scriptName).href; // includes trailing slash
  } catch {
    // Must be a `blob:` or `data:` URL (e.g. `blob:http://site.com/etc/etc`), we cannot
    // infer anything from them.
  }

  {
// include: web_or_worker_shell_read.js
if (ENVIRONMENT_IS_WORKER) {
    readBinary = (url) => {
      var xhr = new XMLHttpRequest();
      xhr.open('GET', url, false);
      xhr.responseType = 'arraybuffer';
      xhr.send(null);
      return new Uint8Array(/** @type{!ArrayBuffer} */(xhr.response));
    };
  }

  readAsync = async (url) => {
    // Fetch has some additional restrictions over XHR, like it can't be used on a file:// url.
    // See https://github.com/github/fetch/pull/92#issuecomment-140665932
    // Cordova or Electron apps are typically loaded from a file:// url.
    // So use XHR on webview if URL is a file URL.
    if (isFileURI(url)) {
      return new Promise((resolve, reject) => {
        var xhr = new XMLHttpRequest();
        xhr.open('GET', url, true);
        xhr.responseType = 'arraybuffer';
        xhr.onload = () => {
          if (xhr.status == 200 || (xhr.status == 0 && xhr.response)) { // file URLs can return 0
            resolve(xhr.response);
            return;
          }
          reject(xhr.status);
        };
        xhr.onerror = reject;
        xhr.send(null);
      });
    }
    var response = await fetch(url, { credentials: 'same-origin' });
    if (response.ok) {
      return response.arrayBuffer();
    }
    throw new Error(response.status + ' : ' + response.url);
  };
// end include: web_or_worker_shell_read.js
  }
} else
{
}

var out = console.log.bind(console);
var err = console.error.bind(console);

// end include: shell.js

// include: preamble.js
// === Preamble library stuff ===

// Documentation for the public APIs defined in this file must be updated in:
//    site/source/docs/api_reference/preamble.js.rst
// A prebuilt local version of the documentation is available at:
//    site/build/text/docs/api_reference/preamble.js.txt
// You can also build docs locally as HTML or other formats in site/
// An online HTML version (which may be of a different version of Emscripten)
//    is up at http://kripken.github.io/emscripten-site/docs/api_reference/preamble.js.html

var wasmBinary;

// Wasm globals

//========================================
// Runtime essentials
//========================================

// whether we are quitting the application. no code should run after this.
// set in exit() and abort()
var ABORT = false;

// set by exit() and abort().  Passed to 'onExit' handler.
// NOTE: This is also used as the process return code in shell environments
// but only when noExitRuntime is false.
var EXITSTATUS;

// In STRICT mode, we only define assert() when ASSERTIONS is set.  i.e. we
// don't define it at all in release modes.  This matches the behaviour of
// MINIMAL_RUNTIME.
// TODO(sbc): Make this the default even without STRICT enabled.
/** @type {function(*, string=)} */
function assert(condition, text) {
  if (!condition) {
    // This build was created without ASSERTIONS defined.  `assert()` should not
    // ever be called in this configuration but in case there are callers in
    // the wild leave this simple abort() implementation here for now.
    abort(text);
  }
}

/**
 * Indicates whether filename is delivered via file protocol (as opposed to http/https)
 * @noinline
 */
var isFileURI = (filename) => filename.startsWith('file://');

// include: runtime_common.js
// include: runtime_exceptions.js
// Base Emscripten EH error class
class EmscriptenEH {}

class EmscriptenSjLj extends EmscriptenEH {}

// end include: runtime_exceptions.js
// include: runtime_debug.js
// end include: runtime_debug.js
// Memory management

var runtimeInitialized = false;



// When ALLOW_MEMORY_GROWTH is enabled, the conversion from Wasm
// memory to ArrayBuffer requires some additional logic.
function getMemoryBuffer() {
  return wasmMemory.buffer;
}

function updateMemoryViews() {
  // If we already have a heap that is resizeable/growable buffer we don't
  // need to do anything in updateMemoryViews.
  if (HEAP8?.buffer?.resizable) return;
  var b = getMemoryBuffer();
  HEAP8 = new Int8Array(b);
  HEAP16 = new Int16Array(b);
  HEAPU8 = new Uint8Array(b);
  HEAPU16 = new Uint16Array(b);
  HEAP32 = new Int32Array(b);
  HEAPU32 = new Uint32Array(b);
  HEAPF32 = new Float32Array(b);
  HEAPF64 = new Float64Array(b);
  HEAP64 = new BigInt64Array(b);
  
}

// include: memoryprofiler.js
// end include: memoryprofiler.js
// end include: runtime_common.js
function preRun() {
  var preRun = Module['preRun'];
  if (preRun) {
    if (typeof preRun == 'function') preRun = [preRun];
    onPreRuns.push(...preRun);
  }
  // Begin ATPRERUNS hooks
  callRuntimeCallbacks(onPreRuns);
  // End ATPRERUNS hooks
}

function initRuntime() {
  runtimeInitialized = true;

  // Begin ATINITS hooks
  if (!Module['noFSInit'] && !FS.initialized) FS.init();
TTY.init();
  // End ATINITS hooks

  wasmExports['__wasm_call_ctors']();

  // Begin ATPOSTCTORS hooks
  FS.ignorePermissions = false;
  // End ATPOSTCTORS hooks

}

function postRun() {

  var postRun = Module['postRun'];
  if (postRun) {
    if (typeof postRun == 'function') postRun = [postRun];
    onPostRuns.push(...postRun);
  }

  // Begin ATPOSTRUNS hooks
  callRuntimeCallbacks(onPostRuns);
  // End ATPOSTRUNS hooks
}

/**
 * @param {string|number=} what
 */
function abort(what) {
  Module['onAbort']?.(what);

  what = `Aborted(${what})`;
  // TODO(sbc): Should we remove printing and leave it up to whoever
  // catches the exception?
  err(what);

  ABORT = true;

  what += '. Build with -sASSERTIONS for more info.';

  // Use a wasm runtime error, because a JS error might be seen as a foreign
  // exception, which means we'd run destructors on it. We need the error to
  // simply make the program stop.
  // FIXME This approach does not work in Wasm EH because it currently does not assume
  // all RuntimeErrors are from traps; it decides whether a RuntimeError is from
  // a trap or not based on a hidden field within the object. So at the moment
  // we don't have a way of throwing a wasm trap from JS. TODO Make a JS API that
  // allows this in the wasm spec.

  // Suppress closure compiler warning here. Closure compiler's builtin extern
  // definition for WebAssembly.RuntimeError claims it takes no arguments even
  // though it can.
  // TODO(https://github.com/google/closure-compiler/pull/3913): Remove if/when upstream closure gets fixed.
  /** @suppress {checkTypes} */
  var e = new WebAssembly.RuntimeError(what);

  // Throw the error whether or not MODULARIZE is set because abort is used
  // in code paths apart from instantiation where an exception is expected
  // to be thrown when abort is called.
  throw e;
}

var wasmBinaryFile;

function findWasmBinary() {
  return locateFile('reVC.wasm');
}

function getBinarySync(file) {
  if (readBinary) {
    return readBinary(file);
  }
  // Throwing a plain string here, even though it not normally advisable since
  // this gets turning into an `abort` in instantiateArrayBuffer.
  throw 'both async and sync fetching of the wasm failed';
}

async function getWasmBinary(binaryFile) {
  // If we don't have the binary yet, load it asynchronously using readAsync.
  if (!wasmBinary) {
    // Fetch the binary using readAsync
    try {
      var response = await readAsync(binaryFile);
      return new Uint8Array(response);
    } catch {
      // Fall back to getBinarySync below;
    }
  }

  // Otherwise, getBinarySync should be able to get it synchronously
  return getBinarySync(binaryFile);
}

async function instantiateArrayBuffer(binaryFile, imports) {
  try {
    var binary = await getWasmBinary(binaryFile);
    var instance = await WebAssembly.instantiate(binary, imports);
    return instance;
  } catch (reason) {
    err(`failed to asynchronously prepare wasm: ${reason}`);

    abort(reason);
  }
}

async function instantiateAsync(binary, binaryFile, imports) {
  if (!binary
      // Don't use streaming for file:// delivered objects in a webview, fetch them synchronously.
      && !isFileURI(binaryFile)
      // Avoid using instantiateStreaming() on Node.js since the `fetch()` API
      // does not support `file://` URLs.
      // See: https://github.com/emscripten-core/emscripten/pull/16917
      && !ENVIRONMENT_IS_NODE
     ) {
    try {
      var response = fetch(binaryFile, { credentials: 'same-origin' });
      var instantiationResult = await WebAssembly.instantiateStreaming(response, imports);
      return instantiationResult;
    } catch (reason) {
      // We expect the most common failure cause to be a bad MIME type for the binary,
      // in which case falling back to ArrayBuffer instantiation should work.
      err(`wasm streaming compile failed: ${reason}`);
      err('falling back to ArrayBuffer instantiation');
      // fall back of instantiateArrayBuffer below
    };
  }
  return instantiateArrayBuffer(binaryFile, imports);
}

function getWasmImports() {
  // instrumenting imports is used in asyncify in two ways: to add assertions
  // that check for proper import use, and for JSPI we use them to set up
  // the Promise API on the import side.
  Asyncify.instrumentWasmImports(wasmImports);
  // prepare imports
  var imports = {
    'env': wasmImports,
    'wasi_snapshot_preview1': wasmImports,
  };
  return imports;
}

// Create the wasm instance.
// Receives the wasm imports, returns the exports.
async function createWasm() {
  // Load the wasm module and create an instance of using native support in the JS engine.
  // handle a generated wasm instance, receiving its exports and
  // performing other necessary setup
  function receiveInstance(instance) {
    wasmExports = instance.exports;

    wasmExports = Asyncify.instrumentWasmExports(wasmExports);

    assignWasmExports(wasmExports);

    updateMemoryViews();

    return wasmExports;
  }

  // Prefer streaming instantiation if available.
  function receiveInstantiationResult(result) {
    // 'result' is a ResultObject object which has both the module and instance.
    // receiveInstance() will swap in the exports (to Module.asm) so they can be called
    // TODO: Due to Closure regression https://github.com/google/closure-compiler/issues/3193, the above line no longer optimizes out down to the following line.
    // When the regression is fixed, can restore the above PTHREADS-enabled path.
    return receiveInstance(result['instance']);
  }

  var info = getWasmImports();

  // User shell pages can write their own Module.instantiateWasm = function(imports, successCallback) callback
  // to manually instantiate the Wasm module themselves. This allows pages to
  // run the instantiation parallel to any other async startup actions they are
  // performing.
  // Also pthreads and wasm workers initialize the wasm instance through this
  // path.
  var instantiateWasm = Module['instantiateWasm'];
  if (instantiateWasm) {
    return new Promise((resolve) => {
        instantiateWasm(info, (inst) => resolve(receiveInstance(inst)));
    });
  }

  wasmBinaryFile ??= findWasmBinary();
  var result = await instantiateAsync(wasmBinary, wasmBinaryFile, info);
  var exports = receiveInstantiationResult(result);
  return exports;
}

// end include: preamble.js

// Begin JS library code


  class ExitStatus {
      name = 'ExitStatus';
      constructor(status) {
        this.message = `Program terminated with exit(${status})`;
        this.status = status;
      }
    }

  /** @type {!Int8Array} */
  var HEAP8;

  var callRuntimeCallbacks = (callbacks) => {
      while (callbacks.length > 0) {
        // Pass the module as the first argument.
        callbacks.shift()(Module);
      }
    };
  var onPostRuns = [];
  var addOnPostRun = (cb) => onPostRuns.push(cb);

  var onPreRuns = [];
  var addOnPreRun = (cb) => onPreRuns.push(cb);


  var noExitRuntime = true;

  var stackRestore = (val) => __emscripten_stack_restore(val);

  var stackSave = () => _emscripten_stack_get_current();

  

  var wasmTableMirror = [];
  
  
  var getWasmTableEntry = (funcPtr) => {
      var func = wasmTableMirror[funcPtr];
      if (!func) {
        /** @suppress {checkTypes} */
        wasmTableMirror[funcPtr] = func = wasmTable.get(funcPtr);
        if (Asyncify.isAsyncExport(func)) {
          wasmTableMirror[funcPtr] = func = Asyncify.makeAsyncFunction(func);
        }
      }
      return func;
    };
  var ___call_sighandler = (fp, sig) => getWasmTableEntry(fp)(sig);

  
  /** @type {!Uint32Array} */
  var HEAPU32;
  class ExceptionInfo {
      // excPtr - Thrown object pointer to wrap. Metadata pointer is calculated from it.
      constructor(excPtr) {
        this.excPtr = excPtr;
        this.ptr = excPtr - 24;
      }
  
      set_type(type) {
        HEAPU32[(((this.ptr)+(4))>>2)] = type;
      }
  
      get_type() {
        return HEAPU32[(((this.ptr)+(4))>>2)];
      }
  
      set_destructor(destructor) {
        HEAPU32[(((this.ptr)+(8))>>2)] = destructor;
      }
  
      get_destructor() {
        return HEAPU32[(((this.ptr)+(8))>>2)];
      }
  
      set_caught(caught) {
        caught = caught ? 1 : 0;
        HEAP8[(this.ptr)+(12)] = caught;
      }
  
      get_caught() {
        return HEAP8[(this.ptr)+(12)] != 0;
      }
  
      set_rethrown(rethrown) {
        rethrown = rethrown ? 1 : 0;
        HEAP8[(this.ptr)+(13)] = rethrown;
      }
  
      get_rethrown() {
        return HEAP8[(this.ptr)+(13)] != 0;
      }
  
      // Initialize native structure fields. Should be called once after allocated.
      init(type, destructor) {
        this.set_adjusted_ptr(0);
        this.set_type(type);
        this.set_destructor(destructor);
      }
  
      set_adjusted_ptr(adjustedPtr) {
        HEAPU32[(((this.ptr)+(16))>>2)] = adjustedPtr;
      }
  
      get_adjusted_ptr() {
        return HEAPU32[(((this.ptr)+(16))>>2)];
      }
    }
  
  var uncaughtExceptionCount = 0;
  
  var __Unwind_RaiseException = (ex) => {
      abort()
    };
  var ___cxa_throw = (ptr, type, destructor) => {
      var info = new ExceptionInfo(ptr);
      // Initialize ExceptionInfo content after it was allocated in __cxa_allocate_exception.
      info.init(type, destructor);
      uncaughtExceptionCount++;
      __Unwind_RaiseException(ptr);
    };

  var PATH = {
  isAbs:(path) => path.charAt(0) === '/',
  splitPath:(filename) => {
        var splitPathRe = /^(\/?|)([\s\S]*?)((?:\.{1,2}|[^\/]+?|)(\.[^.\/]*|))(?:[\/]*)$/;
        return splitPathRe.exec(filename).slice(1);
      },
  normalizeArray:(parts, allowAboveRoot) => {
        // if the path tries to go above the root, `up` ends up > 0
        var up = 0;
        for (var i = parts.length - 1; i >= 0; i--) {
          var last = parts[i];
          if (last === '.') {
            parts.splice(i, 1);
          } else if (last === '..') {
            parts.splice(i, 1);
            up++;
          } else if (up) {
            parts.splice(i, 1);
            up--;
          }
        }
        // if the path is allowed to go above the root, restore leading ..s
        if (allowAboveRoot) {
          for (; up; up--) {
            parts.unshift('..');
          }
        }
        return parts;
      },
  normalize:(path) => {
        var isAbsolute = PATH.isAbs(path),
            trailingSlash = path.slice(-1) === '/';
        // Normalize the path
        path = PATH.normalizeArray(path.split('/').filter((p) => !!p), !isAbsolute).join('/');
        if (!path && !isAbsolute) {
          path = '.';
        }
        if (path && trailingSlash) {
          path += '/';
        }
        return (isAbsolute ? '/' : '') + path;
      },
  dirname:(path) => {
        var result = PATH.splitPath(path),
            root = result[0],
            dir = result[1];
        if (!root && !dir) {
          // No dirname whatsoever
          return '.';
        }
        if (dir) {
          // It has a dirname, strip trailing slash
          dir = dir.slice(0, -1);
        }
        return root + dir;
      },
  basename:(path) => path && path.match(/([^\/]+|\/)\/*$/)[1],
join:(...paths) => PATH.normalize(paths.join('/')),
join2:(l, r) => PATH.normalize(l + '/' + r),
};

var initRandomFill = () => {
    // This block is not needed on v19+ since crypto.getRandomValues is builtin
    if (ENVIRONMENT_IS_NODE) {
      var nodeCrypto = require('node:crypto');
      return (view) => (nodeCrypto.randomFillSync(view), 0);
    }

    return (view) => (crypto.getRandomValues(view), 0);
  };
var randomFill = (view) => (randomFill = initRandomFill())(view);



var PATH_FS = {
resolve:(...args) => {
      var resolvedPath = '',
        resolvedAbsolute = false;
      for (var i = args.length - 1; i >= -1 && !resolvedAbsolute; i--) {
        var path = (i >= 0) ? args[i] : FS.cwd();
        // Skip empty and invalid entries
        if (typeof path != 'string') {
          throw new TypeError('Arguments to path.resolve must be strings');
        } else if (!path) {
          return ''; // an invalid portion invalidates the whole thing
        }
        resolvedPath = path + '/' + resolvedPath;
        resolvedAbsolute = PATH.isAbs(path);
      }
      // At this point the path should be resolved to a full absolute path, but
      // handle relative paths to be safe (might happen when process.cwd() fails)
      resolvedPath = PATH.normalizeArray(resolvedPath.split('/').filter((p) => !!p), !resolvedAbsolute).join('/');
      return ((resolvedAbsolute ? '/' : '') + resolvedPath) || '.';
    },
relative:(from, to) => {
      from = PATH_FS.resolve(from).slice(1);
      to = PATH_FS.resolve(to).slice(1);
      function trim(arr) {
        var start = 0;
        for (; start < arr.length; start++) {
          if (arr[start] !== '') break;
        }
        var end = arr.length - 1;
        for (; end >= 0; end--) {
          if (arr[end] !== '') break;
        }
        if (start > end) return [];
        return arr.slice(start, end - start + 1);
      }
      var fromParts = trim(from.split('/'));
      var toParts = trim(to.split('/'));
      var length = Math.min(fromParts.length, toParts.length);
      var samePartsLength = length;
      for (var i = 0; i < length; i++) {
        if (fromParts[i] !== toParts[i]) {
          samePartsLength = i;
          break;
        }
      }
      var outputParts = [];
      for (var i = samePartsLength; i < fromParts.length; i++) {
        outputParts.push('..');
      }
      outputParts = outputParts.concat(toParts.slice(samePartsLength));
      return outputParts.join('/');
    },
};


var UTF8Decoder = globalThis.TextDecoder && new TextDecoder();


  /**
   * heapOrArray is either a regular array, or a JavaScript typed array view.
   * @param {number} idx
   * @param {number=} maxBytesToRead
   * @param {boolean=} ignoreNul
   * @return {number}
   */
  var findStringEnd = (heapOrArray, idx, maxBytesToRead, ignoreNul) => {
      var maxIdx = idx + maxBytesToRead;
      if (ignoreNul) return maxIdx;
      // TextDecoder needs to know the byte length in advance, it doesn't stop on
      // null terminator by itself.
      // As a tiny code save trick, compare idx against maxIdx using a negation,
      // so that maxBytesToRead=undefined/NaN means Infinity.
      while (heapOrArray[idx] && !(idx >= maxIdx)) ++idx;
      return idx;
    };
  
    /**
   * Given a pointer 'idx' to a null-terminated UTF8-encoded string in the given
   * array that contains uint8 values, returns a copy of that string as a
   * Javascript String object.
   * heapOrArray is either a regular array, or a JavaScript typed array view.
   * @param {number=} idx
   * @param {number=} maxBytesToRead
   * @param {boolean=} ignoreNul - If true, the function will not stop on a NUL character.
   * @return {string}
   */
  var UTF8ArrayToString = (heapOrArray, idx = 0, maxBytesToRead, ignoreNul) => {
  
      var endPtr = findStringEnd(heapOrArray, idx, maxBytesToRead, ignoreNul);
  
      // When using conditional TextDecoder, skip it for short strings as the overhead of the native call is not worth it.
      if (endPtr - idx > 16 && heapOrArray.buffer && UTF8Decoder) {
        return UTF8Decoder.decode(heapOrArray.subarray(idx, endPtr));
      }
      var str = '';
      while (idx < endPtr) {
        // For UTF8 byte structure, see:
        // http://en.wikipedia.org/wiki/UTF-8#Description
        // https://www.ietf.org/rfc/rfc2279.txt
        // https://tools.ietf.org/html/rfc3629
        var u0 = heapOrArray[idx++];
        if (!(u0 & 0x80)) { str += String.fromCharCode(u0); continue; }
        var u1 = heapOrArray[idx++] & 63;
        if ((u0 & 0xE0) == 0xC0) { str += String.fromCharCode(((u0 & 31) << 6) | u1); continue; }
        var u2 = heapOrArray[idx++] & 63;
        if ((u0 & 0xF0) == 0xE0) {
          u0 = ((u0 & 15) << 12) | (u1 << 6) | u2;
        } else {
          u0 = ((u0 & 7) << 18) | (u1 << 12) | (u2 << 6) | (heapOrArray[idx++] & 63);
        }
  
        if (u0 < 0x10000) {
          str += String.fromCharCode(u0);
        } else {
          var ch = u0 - 0x10000;
          str += String.fromCharCode(0xD800 | (ch >> 10), 0xDC00 | (ch & 0x3FF));
        }
      }
      return str;
    };
  
  var FS_stdin_getChar_buffer = [];
  
  var lengthBytesUTF8 = (str) => {
      var len = 0;
      for (var i = 0; i < str.length; ++i) {
        // Gotcha: charCodeAt returns a 16-bit word that is a UTF-16 encoded code
        // unit, not a Unicode code point of the character! So decode
        // UTF16->UTF32->UTF8.
        // See http://unicode.org/faq/utf_bom.html#utf16-3
        var c = str.charCodeAt(i); // possibly a lead surrogate
        if (c <= 0x7F) {
          len++;
        } else if (c <= 0x7FF) {
          len += 2;
        } else if (c >= 0xD800 && c <= 0xDFFF) {
          len += 4; ++i;
        } else {
          len += 3;
        }
      }
      return len;
    };
  
  var stringToUTF8Array = (str, heap, outIdx, maxBytesToWrite) => {
      // Parameter maxBytesToWrite is not optional. Negative values, 0, null,
      // undefined and false each don't write out any bytes.
      if (!(maxBytesToWrite > 0))
        return 0;
  
      var startIdx = outIdx;
      var endIdx = outIdx + maxBytesToWrite - 1; // -1 for string null terminator.
      for (var i = 0; i < str.length; ++i) {
        // For UTF8 byte structure, see http://en.wikipedia.org/wiki/UTF-8#Description
        // and https://www.ietf.org/rfc/rfc2279.txt
        // and https://tools.ietf.org/html/rfc3629
        var u = str.codePointAt(i);
        if (u <= 0x7F) {
          if (outIdx >= endIdx) break;
          heap[outIdx++] = u;
        } else if (u <= 0x7FF) {
          if (outIdx + 1 >= endIdx) break;
          heap[outIdx++] = 0xC0 | (u >> 6);
          heap[outIdx++] = 0x80 | (u & 63);
        } else if (u <= 0xFFFF) {
          if (outIdx + 2 >= endIdx) break;
          heap[outIdx++] = 0xE0 | (u >> 12);
          heap[outIdx++] = 0x80 | ((u >> 6) & 63);
          heap[outIdx++] = 0x80 | (u & 63);
        } else {
          if (outIdx + 3 >= endIdx) break;
          heap[outIdx++] = 0xF0 | (u >> 18);
          heap[outIdx++] = 0x80 | ((u >> 12) & 63);
          heap[outIdx++] = 0x80 | ((u >> 6) & 63);
          heap[outIdx++] = 0x80 | (u & 63);
          // Gotcha: if codePoint is over 0xFFFF, it is represented as a surrogate pair in UTF-16.
          // We need to manually skip over the second code unit for correct iteration.
          i++;
        }
      }
      // Null-terminate the pointer to the buffer.
      heap[outIdx] = 0;
      return outIdx - startIdx;
    };
  /** @type {function(string, boolean=, number=)} */
  var intArrayFromString = (stringy, dontAddNull, length) => {
      var len = length > 0 ? length : lengthBytesUTF8(stringy)+1;
      var u8array = new Array(len);
      var numBytesWritten = stringToUTF8Array(stringy, u8array, 0, u8array.length);
      if (dontAddNull) u8array.length = numBytesWritten;
      return u8array;
    };
  var FS_stdin_getChar = () => {
      if (!FS_stdin_getChar_buffer.length) {
        var result = null;
        if (ENVIRONMENT_IS_NODE) {
          // we will read data by chunks of BUFSIZE
          var BUFSIZE = 256;
          var buf = Buffer.alloc(BUFSIZE);
          var bytesRead = 0;
  
          // For some reason we must suppress a closure warning here, even though
          // fd definitely exists on process.stdin, and is even the proper way to
          // get the fd of stdin,
          // https://github.com/nodejs/help/issues/2136#issuecomment-523649904
          // This started to happen after moving this logic out of library_tty.js,
          // so it is related to the surrounding code in some unclear manner.
          /** @suppress {missingProperties} */
          var fd = process.stdin.fd;
  
          try {
            bytesRead = fs.readSync(fd, buf, 0, BUFSIZE);
          } catch(e) {
            // Cross-platform differences: on Windows, reading EOF throws an
            // exception, but on other OSes, reading EOF returns 0. Uniformize
            // behavior by treating the EOF exception to return 0.
            if (e.toString().includes('EOF')) bytesRead = 0;
            else throw e;
          }
  
          if (bytesRead > 0) {
            result = buf.slice(0, bytesRead).toString('utf-8');
          }
        } else
        if (globalThis.window?.prompt) {
          // Browser.
          result = window.prompt('Input: ');  // returns null on cancel
          if (result !== null) {
            result += '\n';
          }
        } else
        {}
        if (!result) {
          return null;
        }
        FS_stdin_getChar_buffer = intArrayFromString(result, true);
      }
      return FS_stdin_getChar_buffer.shift();
    };
  var TTY = {
  ttys:[],
  init() {
        // https://github.com/emscripten-core/emscripten/pull/1555
        // if (ENVIRONMENT_IS_NODE) {
        //   // currently, FS.init does not distinguish if process.stdin is a file or TTY
        //   // device, it always assumes it's a TTY device. because of this, we're forcing
        //   // process.stdin to UTF8 encoding to at least make stdin reading compatible
        //   // with text files until FS.init can be refactored.
        //   process.stdin.setEncoding('utf8');
        // }
      },
  shutdown() {
        // https://github.com/emscripten-core/emscripten/pull/1555
        // if (ENVIRONMENT_IS_NODE) {
        //   // inolen: any idea as to why node -e 'process.stdin.read()' wouldn't exit immediately (with process.stdin being a tty)?
        //   // isaacs: because now it's reading from the stream, you've expressed interest in it, so that read() kicks off a _read() which creates a ReadReq operation
        //   // inolen: I thought read() in that case was a synchronous operation that just grabbed some amount of buffered data if it exists?
        //   // isaacs: it is. but it also triggers a _read() call, which calls readStart() on the handle
        //   // isaacs: do process.stdin.pause() and i'd think it'd probably close the pending call
        //   process.stdin.pause();
        // }
      },
  register(dev, ops) {
        TTY.ttys[dev] = { input: [], output: [], ops: ops };
        FS.registerDevice(dev, TTY.stream_ops);
      },
  stream_ops:{
  open(stream) {
          var tty = TTY.ttys[stream.node.rdev];
          if (!tty) {
            throw new FS.ErrnoError(43);
          }
          stream.tty = tty;
          stream.seekable = false;
        },
  close(stream) {
          // flush any pending line data
          stream.tty.ops.fsync(stream.tty);
        },
  fsync(stream) {
          stream.tty.ops.fsync(stream.tty);
        },
  read(stream, buffer, offset, length, pos /* ignored */) {
          if (!stream.tty || !stream.tty.ops.get_char) {
            throw new FS.ErrnoError(60);
          }
          var bytesRead = 0;
          for (var i = 0; i < length; i++) {
            var result;
            try {
              result = stream.tty.ops.get_char(stream.tty);
            } catch (e) {
              throw new FS.ErrnoError(29);
            }
            if (result === undefined && !bytesRead) {
              throw new FS.ErrnoError(6);
            }
            if (result === null || result === undefined) break;
            bytesRead++;
            buffer[offset+i] = result;
            // We currently only support canonical mode (ICANON), where
            // read(2) returns as soon as a line delimiter is read.
            if (result === 10) break;
          }
          if (bytesRead) {
            stream.node.atime = Date.now();
          }
          return bytesRead;
        },
  write(stream, buffer, offset, length, pos) {
          if (!stream.tty || !stream.tty.ops.put_char) {
            throw new FS.ErrnoError(60);
          }
          try {
            for (var i = 0; i < length; i++) {
              stream.tty.ops.put_char(stream.tty, buffer[offset+i]);
            }
          } catch (e) {
            throw new FS.ErrnoError(29);
          }
          if (length) {
            stream.node.mtime = stream.node.ctime = Date.now();
          }
          return i;
        },
  },
  default_tty_ops:{
  get_char(tty) {
          return FS_stdin_getChar();
        },
  put_char(tty, val) {
          if (val === null || val === 10) {
            out(UTF8ArrayToString(tty.output));
            tty.output = [];
          } else {
            if (val != 0) tty.output.push(val); // val == 0 would cut text output off in the middle.
          }
        },
  fsync(tty) {
          if (tty.output?.length > 0) {
            out(UTF8ArrayToString(tty.output));
            tty.output = [];
          }
        },
  ioctl_tcgets(tty) {
          // typical setting
          return {
            c_iflag: 25856,
            c_oflag: 5,
            c_cflag: 191,
            c_lflag: 35387,
            c_cc: [
              0x03, 0x1c, 0x7f, 0x15, 0x04, 0x00, 0x01, 0x00, 0x11, 0x13, 0x1a, 0x00,
              0x12, 0x0f, 0x17, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            ]
          };
        },
  ioctl_tcsets(tty, optional_actions, data) {
          // currently just ignore
          return 0;
        },
  ioctl_tiocgwinsz(tty) {
          return [24, 80];
        },
  },
  default_tty1_ops:{
  put_char(tty, val) {
          if (val === null || val === 10) {
            err(UTF8ArrayToString(tty.output));
            tty.output = [];
          } else {
            if (val != 0) tty.output.push(val);
          }
        },
  fsync(tty) {
          if (tty.output?.length > 0) {
            err(UTF8ArrayToString(tty.output));
            tty.output = [];
          }
        },
  },
  };
  
  
  var mmapAlloc = (size) => {
      abort();
    };
  
  var MEMFS = {
  ops_table:null,
  mount(mount) {
        return MEMFS.createNode(null, '/', 16895, 0);
      },
  createNode(parent, name, mode, dev) {
        if (FS.isBlkdev(mode) || FS.isFIFO(mode)) {
          // not supported
          throw new FS.ErrnoError(63);
        }
        MEMFS.ops_table ||= {
          dir: {
            node: {
              getattr: MEMFS.node_ops.getattr,
              setattr: MEMFS.node_ops.setattr,
              lookup: MEMFS.node_ops.lookup,
              mknod: MEMFS.node_ops.mknod,
              rename: MEMFS.node_ops.rename,
              unlink: MEMFS.node_ops.unlink,
              rmdir: MEMFS.node_ops.rmdir,
              readdir: MEMFS.node_ops.readdir,
              symlink: MEMFS.node_ops.symlink
            },
            stream: {
              llseek: MEMFS.stream_ops.llseek
            }
          },
          file: {
            node: {
              getattr: MEMFS.node_ops.getattr,
              setattr: MEMFS.node_ops.setattr
            },
            stream: {
              llseek: MEMFS.stream_ops.llseek,
              read: MEMFS.stream_ops.read,
              write: MEMFS.stream_ops.write,
              mmap: MEMFS.stream_ops.mmap,
              msync: MEMFS.stream_ops.msync
            }
          },
          link: {
            node: {
              getattr: MEMFS.node_ops.getattr,
              setattr: MEMFS.node_ops.setattr,
              readlink: MEMFS.node_ops.readlink
            },
            stream: {}
          },
          chrdev: {
            node: {
              getattr: MEMFS.node_ops.getattr,
              setattr: MEMFS.node_ops.setattr
            },
            stream: FS.chrdev_stream_ops
          }
        };
        var node = FS.createNode(parent, name, mode, dev);
        if (FS.isDir(node.mode)) {
          node.node_ops = MEMFS.ops_table.dir.node;
          node.stream_ops = MEMFS.ops_table.dir.stream;
          node.contents = {};
        } else if (FS.isFile(node.mode)) {
          node.node_ops = MEMFS.ops_table.file.node;
          node.stream_ops = MEMFS.ops_table.file.stream;
          // The actual number of bytes used in the typed array, as opposed to
          // contents.length which gives the whole capacity.
          node.usedBytes = 0;
          // The byte data of the file is stored in a typed array.
          // Note: typed arrays are not resizable like normal JS arrays are, so
          // there is a small penalty involved for appending file writes that
          // continuously grow a file similar to std::vector capacity vs used.
          node.contents = MEMFS.emptyFileContents ??= new Uint8Array(0);
        } else if (FS.isLink(node.mode)) {
          node.node_ops = MEMFS.ops_table.link.node;
          node.stream_ops = MEMFS.ops_table.link.stream;
        } else if (FS.isChrdev(node.mode)) {
          node.node_ops = MEMFS.ops_table.chrdev.node;
          node.stream_ops = MEMFS.ops_table.chrdev.stream;
        }
        node.atime = node.mtime = node.ctime = Date.now();
        // add the new node to the parent
        if (parent) {
          parent.contents[name] = node;
          parent.atime = parent.mtime = parent.ctime = node.atime;
        }
        return node;
      },
  getFileDataAsTypedArray(node) {
        return node.contents.subarray(0, node.usedBytes); // Make sure to not return excess unused bytes.
      },
  expandFileStorage(node, newCapacity) {
        var prevCapacity = node.contents.length;
        if (prevCapacity >= newCapacity) return; // No need to expand, the storage was already large enough.
        // Don't expand strictly to the given requested limit if it's only a very
        // small increase, but instead geometrically grow capacity.
        // For small filesizes (<1MB), perform size*2 geometric increase, but for
        // large sizes, do a much more conservative size*1.125 increase to avoid
        // overshooting the allocation cap by a very large margin.
        var CAPACITY_DOUBLING_MAX = 1024 * 1024;
        newCapacity = Math.max(newCapacity, (prevCapacity * (prevCapacity < CAPACITY_DOUBLING_MAX ? 2.0 : 1.125)) >>> 0);
        if (prevCapacity) newCapacity = Math.max(newCapacity, 256); // At minimum allocate 256b for each file when expanding.
        var oldContents = MEMFS.getFileDataAsTypedArray(node);
        node.contents = new Uint8Array(newCapacity); // Allocate new storage.
        node.contents.set(oldContents);
      },
  resizeFileStorage(node, newSize) {
        if (node.usedBytes == newSize) return;
        var oldContents = node.contents;
        node.contents = new Uint8Array(newSize); // Allocate new storage.
        node.contents.set(oldContents.subarray(0, Math.min(newSize, node.usedBytes))); // Copy old data over to the new storage.
        node.usedBytes = newSize;
      },
  node_ops:{
  getattr(node) {
          var attr = {};
          // device numbers reuse inode numbers.
          attr.dev = FS.isChrdev(node.mode) ? node.id : 1;
          attr.ino = node.id;
          attr.mode = node.mode;
          attr.nlink = 1;
          attr.uid = 0;
          attr.gid = 0;
          attr.rdev = node.rdev;
          if (FS.isDir(node.mode)) {
            attr.size = 4096;
          } else if (FS.isFile(node.mode)) {
            attr.size = node.usedBytes;
          } else if (FS.isLink(node.mode)) {
            attr.size = node.link.length;
          } else {
            attr.size = 0;
          }
          attr.atime = new Date(node.atime);
          attr.mtime = new Date(node.mtime);
          attr.ctime = new Date(node.ctime);
          // NOTE: In our implementation, st_blocks = Math.ceil(st_size/st_blksize),
          //       but this is not required by the standard.
          attr.blksize = 4096;
          attr.blocks = Math.ceil(attr.size / attr.blksize);
          return attr;
        },
  setattr(node, attr) {
          for (const key of ['mode', 'atime', 'mtime', 'ctime']) {
            if (attr[key] != null) {
              node[key] = attr[key];
            }
          }
          if (attr.size !== undefined) {
            MEMFS.resizeFileStorage(node, attr.size);
          }
        },
  lookup(parent, name) {
          // This error may happen quite a bit. To avoid overhead we reuse it (and
          // suffer a lack of stack info).
          if (!MEMFS.doesNotExistError) {
            MEMFS.doesNotExistError = new FS.ErrnoError(44);
            /** @suppress {checkTypes} */
            MEMFS.doesNotExistError.stack = '<generic error, no stack>';
          }
          throw MEMFS.doesNotExistError;
        },
  mknod(parent, name, mode, dev) {
          return MEMFS.createNode(parent, name, mode, dev);
        },
  rename(old_node, new_dir, new_name) {
          var new_node;
          try {
            new_node = FS.lookupNode(new_dir, new_name);
          } catch (e) {}
          if (new_node) {
            if (FS.isDir(old_node.mode)) {
              // if we're overwriting a directory at new_name, make sure it's empty.
              for (var i in new_node.contents) {
                throw new FS.ErrnoError(55);
              }
            }
            FS.hashRemoveNode(new_node);
          }
          // do the internal rewiring
          delete old_node.parent.contents[old_node.name];
          new_dir.contents[new_name] = old_node;
          old_node.name = new_name;
          new_dir.ctime = new_dir.mtime = old_node.parent.ctime = old_node.parent.mtime = Date.now();
        },
  unlink(parent, name) {
          delete parent.contents[name];
          parent.ctime = parent.mtime = Date.now();
        },
  rmdir(parent, name) {
          var node = FS.lookupNode(parent, name);
          for (var i in node.contents) {
            throw new FS.ErrnoError(55);
          }
          delete parent.contents[name];
          parent.ctime = parent.mtime = Date.now();
        },
  readdir(node) {
          return ['.', '..', ...Object.keys(node.contents)];
        },
  symlink(parent, newname, oldpath) {
          var node = MEMFS.createNode(parent, newname, 0o777 | 40960, 0);
          node.link = oldpath;
          return node;
        },
  readlink(node) {
          if (!FS.isLink(node.mode)) {
            throw new FS.ErrnoError(28);
          }
          return node.link;
        },
  },
  stream_ops:{
  read(stream, buffer, offset, length, position) {
          var contents = stream.node.contents;
          if (position >= stream.node.usedBytes) return 0;
          var size = Math.min(stream.node.usedBytes - position, length);
          buffer.set(contents.subarray(position, position + size), offset);
          return size;
        },
  write(stream, buffer, offset, length, position, canOwn) {
          // If the buffer is located in main memory (HEAP), and if
          // memory can grow, we can't hold on to references of the
          // memory buffer, as they may get invalidated. That means we
          // need to copy its contents.
          if (buffer.buffer === HEAP8.buffer) {
            canOwn = false;
          }
  
          if (!length) return 0;
          var node = stream.node;
          node.mtime = node.ctime = Date.now();
  
          if (canOwn) {
            node.contents = buffer.subarray(offset, offset + length);
            node.usedBytes = length;
          } else if (!node.usedBytes && !position) { // If this is a simple first write to an empty file, do a fast set since we don't need to care about old data.
            node.contents = buffer.slice(offset, offset + length);
            node.usedBytes = length;
          } else {
            MEMFS.expandFileStorage(node, position+length);
            // Use typed array write which is available.
            node.contents.set(buffer.subarray(offset, offset + length), position);
            node.usedBytes = Math.max(node.usedBytes, position + length);
          }
          return length;
        },
  llseek(stream, offset, whence) {
          var position = offset;
          if (whence === 1) {
            position += stream.position;
          } else if (whence === 2) {
            if (FS.isFile(stream.node.mode)) {
              position += stream.node.usedBytes;
            }
          }
          if (position < 0) {
            throw new FS.ErrnoError(28);
          }
          return position;
        },
  mmap(stream, length, position, prot, flags) {
          if (!FS.isFile(stream.node.mode)) {
            throw new FS.ErrnoError(43);
          }
          var ptr;
          var allocated;
          var contents = stream.node.contents;
          // Only make a new copy when MAP_PRIVATE is specified.
          if (!(flags & 2) && contents.buffer === HEAP8.buffer) {
            // We can't emulate MAP_SHARED when the file is not backed by the
            // buffer we're mapping to (e.g. the HEAP buffer).
            allocated = false;
            ptr = contents.byteOffset;
          } else {
            allocated = true;
            ptr = mmapAlloc(length);
            if (!ptr) {
              throw new FS.ErrnoError(48);
            }
            if (contents) {
              // Try to avoid unnecessary slices.
              if (position > 0 || position + length < contents.length) {
                if (contents.subarray) {
                  contents = contents.subarray(position, position + length);
                } else {
                  contents = Array.prototype.slice.call(contents, position, position + length);
                }
              }
              HEAP8.set(contents, ptr);
            }
          }
          return { ptr, allocated };
        },
  msync(stream, buffer, offset, length, mmapFlags) {
          MEMFS.stream_ops.write(stream, buffer, 0, length, offset, false);
          // should we check if bytesWritten and length are the same?
          return 0;
        },
  },
  };
  
  var FS_modeStringToFlags = (str) => {
      if (typeof str != 'string') return str;
      var flagModes = {
        'r': 0,
        'r+': 2,
        'w': 512 | 64 | 1,
        'w+': 512 | 64 | 2,
        'a': 1024 | 64 | 1,
        'a+': 1024 | 64 | 2,
      };
      var flags = flagModes[str];
      if (typeof flags == 'undefined') {
        throw new Error(`Unknown file open mode: ${str}`);
      }
      return flags;
    };
  
  var FS_fileDataToTypedArray = (data) => {
      if (typeof data == 'string') {
        data = intArrayFromString(data, true);
      }
      if (!data.subarray) {
        data = new Uint8Array(data);
      }
      return data;
    };
  
  var FS_getMode = (canRead, canWrite) => {
      var mode = 0;
      if (canRead) mode |= 292 | 73;
      if (canWrite) mode |= 146;
      return mode;
    };
  
  
  
  
  var IDBFS = {
  dbs:{
  },
  indexedDB:() => {
        return indexedDB;
      },
  DB_VERSION:21,
  DB_STORE_NAME:"FILE_DATA",
  queuePersist:(mount) => {
        function onPersistComplete() {
          if (mount.idbPersistState === 'again') startPersist(); // If a new sync request has appeared in between, kick off a new sync
          else {
            mount.idbPersistState = 0; // Otherwise reset sync state back to idle to wait for a new sync later
            IDBFS.onAutoPersistStateChanged?.(false);
          }
        }
        function startPersist() {
          mount.idbPersistState = 'idb'; // Mark that we are currently running a sync operation
          IDBFS.onAutoPersistStateChanged?.(true);
          IDBFS.syncfs(mount, /*populate:*/false, onPersistComplete);
        }
  
        if (!mount.idbPersistState) {
          // Programs typically write/copy/move multiple files in the in-memory
          // filesystem within a single app frame, so when a filesystem sync
          // command is triggered, do not start it immediately, but only after
          // the current frame is finished. This way all the modified files
          // inside the main loop tick will be batched up to the same sync.
          mount.idbPersistState = setTimeout(startPersist, 0);
        } else if (mount.idbPersistState === 'idb') {
          // There is an active IndexedDB sync operation in-flight, but we now
          // have accumulated more files to sync. We should therefore queue up
          // a new sync after the current one finishes so that all writes
          // will be properly persisted.
          mount.idbPersistState = 'again';
        }
      },
  mount:(mount) => {
        // reuse core MEMFS functionality
        var mnt = MEMFS.mount(mount);
        // If the automatic IDBFS persistence option has been selected, then automatically persist
        // all modifications to the filesystem as they occur.
        if (mount?.opts?.autoPersist) {
          mount.idbPersistState = 0; // IndexedDB sync starts in idle state
          var memfs_node_ops = mnt.node_ops;
          mnt.node_ops = {...mnt.node_ops}; // Clone node_ops to inject write tracking
          mnt.node_ops.mknod = (parent, name, mode, dev) => {
            var node = memfs_node_ops.mknod(parent, name, mode, dev);
            // Propagate injected node_ops to the newly created child node
            node.node_ops = mnt.node_ops;
            // Remember for each IDBFS node which IDBFS mount point they came from so we know which mount to persist on modification.
            node.idbfs_mount = mnt.mount;
            // Remember original MEMFS stream_ops for this node
            node.memfs_stream_ops = node.stream_ops;
            // Clone stream_ops to inject write tracking
            node.stream_ops = {...node.stream_ops};
  
            // Track all file writes
            node.stream_ops.write = (stream, buffer, offset, length, position, canOwn) => {
              // This file has been modified, we must persist IndexedDB when this file closes
              stream.node.isModified = true;
              return node.memfs_stream_ops.write(stream, buffer, offset, length, position, canOwn);
            };
  
            // Persist IndexedDB on file close
            node.stream_ops.close = (stream) => {
              var n = stream.node;
              if (n.isModified) {
                IDBFS.queuePersist(n.idbfs_mount);
                n.isModified = false;
              }
              if (n.memfs_stream_ops.close) return n.memfs_stream_ops.close(stream);
            };
  
            // Persist the node we just created to IndexedDB
            IDBFS.queuePersist(mnt.mount);
  
            return node;
          };
          // Also kick off persisting the filesystem on other operations that modify the filesystem.
          mnt.node_ops.rmdir   = (...args) => (IDBFS.queuePersist(mnt.mount), memfs_node_ops.rmdir(...args));
          mnt.node_ops.symlink = (...args) => (IDBFS.queuePersist(mnt.mount), memfs_node_ops.symlink(...args));
          mnt.node_ops.unlink  = (...args) => (IDBFS.queuePersist(mnt.mount), memfs_node_ops.unlink(...args));
          mnt.node_ops.rename  = (...args) => (IDBFS.queuePersist(mnt.mount), memfs_node_ops.rename(...args));
        }
        return mnt;
      },
  syncfs:(mount, populate, callback) => {
        IDBFS.getLocalSet(mount, (err, local) => {
          if (err) return callback(err);
  
          IDBFS.getRemoteSet(mount, (err, remote) => {
            if (err) return callback(err);
  
            var src = populate ? remote : local;
            var dst = populate ? local : remote;
  
            IDBFS.reconcile(src, dst, callback);
          });
        });
      },
  quit:() => {
        for (var value of Object.values(IDBFS.dbs)) {
          value.close()
        }
        IDBFS.dbs = {};
      },
  getDB:(name, callback) => {
        // check the cache first
        var db = IDBFS.dbs[name];
        if (db) {
          return callback(null, db);
        }
  
        var req;
        try {
          req = IDBFS.indexedDB().open(name, IDBFS.DB_VERSION);
        } catch (e) {
          return callback(e);
        }
        if (!req) {
          return callback('Unable to connect to IndexedDB');
        }
        req.onupgradeneeded = (e) => {
          var db = /** @type {IDBDatabase} */ (e.target.result);
          var transaction = e.target.transaction;
  
          var fileStore;
  
          if (db.objectStoreNames.contains(IDBFS.DB_STORE_NAME)) {
            fileStore = transaction.objectStore(IDBFS.DB_STORE_NAME);
          } else {
            fileStore = db.createObjectStore(IDBFS.DB_STORE_NAME);
          }
  
          if (!fileStore.indexNames.contains('timestamp')) {
            fileStore.createIndex('timestamp', 'timestamp', { unique: false });
          }
        };
        req.onsuccess = () => {
          db = /** @type {IDBDatabase} */ (req.result);
  
          // add to the cache
          IDBFS.dbs[name] = db;
          callback(null, db);
        };
        req.onerror = (e) => {
          callback(e.target.error);
          e.preventDefault();
        };
      },
  getLocalSet:(mount, callback) => {
        var entries = {};
  
        function isRealDir(p) {
          return p !== '.' && p !== '..';
        };
        function toAbsolute(root) {
          return (p) => PATH.join2(root, p);
        };
  
        var check = FS.readdir(mount.mountpoint).filter(isRealDir).map(toAbsolute(mount.mountpoint));
  
        while (check.length) {
          var path = check.pop();
          var stat;
  
          try {
            stat = FS.lstat(path);
          } catch (e) {
            return callback(e);
          }
  
          if (FS.isDir(stat.mode)) {
            check.push(...FS.readdir(path).filter(isRealDir).map(toAbsolute(path)));
          }
  
          entries[path] = { 'timestamp': stat.mtime };
        }
  
        return callback(null, { type: 'local', entries: entries });
      },
  getRemoteSet:(mount, callback) => {
        var entries = {};
  
        IDBFS.getDB(mount.mountpoint, (err, db) => {
          if (err) return callback(err);
  
          try {
            var transaction = db.transaction([IDBFS.DB_STORE_NAME], 'readonly');
            transaction.onerror = (e) => {
              callback(e.target.error);
              e.preventDefault();
            };
  
            var store = transaction.objectStore(IDBFS.DB_STORE_NAME);
            var index = store.index('timestamp');
  
            index.openKeyCursor().onsuccess = (event) => {
              var cursor = event.target.result;
  
              if (!cursor) {
                return callback(null, { type: 'remote', db, entries });
              }
  
              entries[cursor.primaryKey] = { 'timestamp': cursor.key };
  
              cursor.continue();
            };
          } catch (e) {
            return callback(e);
          }
        });
      },
  loadLocalEntry:(path, callback) => {
        var stat, node;
  
        try {
          var lookup = FS.lookupPath(path);
          node = lookup.node;
          stat = FS.lstat(path);
        } catch (e) {
          return callback(e);
        }
  
        if (FS.isDir(stat.mode)) {
          return callback(null, { 'timestamp': stat.mtime, 'mode': stat.mode });
        } else if (FS.isLink(stat.mode)) {
          return callback(null, { 'timestamp': stat.mtime, 'mode': stat.mode, 'link': node.link, });
        } else if (FS.isFile(stat.mode)) {
          // Performance consideration: storing a normal JavaScript array to a IndexedDB is much slower than storing a typed array.
          // Therefore always convert the file contents to a typed array first before writing the data to IndexedDB.
          node.contents = MEMFS.getFileDataAsTypedArray(node);
          return callback(null, { 'timestamp': stat.mtime, 'mode': stat.mode, 'contents': node.contents });
        } else {
          return callback(new Error('node type not supported'));
        }
      },
  storeLocalEntry:(path, entry, callback) => {
        try {
          if (FS.isDir(entry['mode'])) {
            FS.mkdirTree(path, entry['mode']);
          } else if (FS.isLink(entry['mode'])) {
            FS.symlink(entry['link'], path);
          } else if (FS.isFile(entry['mode'])) {
            FS.writeFile(path, entry['contents'], { canOwn: true });
          } else {
            return callback(new Error('node type not supported'));
          }
  
          FS.chmod(path, entry['mode']);
          FS.utime(path, entry['timestamp'], entry['timestamp']);
        } catch (e) {
          return callback(e);
        }
  
        callback(null);
      },
  removeLocalEntry:(path, callback) => {
        try {
          var stat = FS.lstat(path);
  
          if (FS.isDir(stat.mode)) {
            FS.rmdir(path);
          } else {
            FS.unlink(path);
          }
        } catch (e) {
          return callback(e);
        }
  
        callback(null);
      },
  loadRemoteEntry:(store, path, callback) => {
        var req = store.get(path);
        req.onsuccess = (event) => callback(null, event.target.result);
        req.onerror = (e) => {
          callback(e.target.error);
          e.preventDefault();
        };
      },
  storeRemoteEntry:(store, path, entry, callback) => {
        try {
          var req = store.put(entry, path);
        } catch (e) {
          callback(e);
          return;
        }
        req.onsuccess = (event) => callback();
        req.onerror = (e) => {
          callback(e.target.error);
          e.preventDefault();
        };
      },
  removeRemoteEntry:(store, path, callback) => {
        var req = store.delete(path);
        req.onsuccess = (event) => callback();
        req.onerror = (e) => {
          callback(e.target.error);
          e.preventDefault();
        };
      },
  reconcile:(src, dst, callback) => {
        var total = 0;
  
        var create = [];
        for (var [key, e] of Object.entries(src.entries)) {
          var e2 = dst.entries[key];
          if (!e2 || e['timestamp'].getTime() != e2['timestamp'].getTime()) {
            create.push(key);
            total++;
          }
        }
  
        var remove = [];
        for (var key of Object.keys(dst.entries)) {
          if (!src.entries[key]) {
            remove.push(key);
            total++;
          }
        }
  
        if (!total) {
          return callback(null);
        }
  
        var errored = false;
        var db = src.type === 'remote' ? src.db : dst.db;
        var transaction = db.transaction([IDBFS.DB_STORE_NAME], 'readwrite');
        var store = transaction.objectStore(IDBFS.DB_STORE_NAME);
  
        function done(err) {
          if (err && !errored) {
            errored = true;
            return callback(err);
          }
        };
  
        // transaction may abort if (for example) there is a QuotaExceededError
        transaction.onerror = transaction.onabort = (e) => {
          done(e.target.error);
          e.preventDefault();
        };
  
        transaction.oncomplete = (e) => {
          if (!errored) {
            callback(null);
          }
        };
  
        // sort paths in ascending order so directory entries are created
        // before the files inside them
        for (const path of create.sort()) {
          if (dst.type === 'local') {
            IDBFS.loadRemoteEntry(store, path, (err, entry) => {
              if (err) return done(err);
              IDBFS.storeLocalEntry(path, entry, done);
            });
          } else {
            IDBFS.loadLocalEntry(path, (err, entry) => {
              if (err) return done(err);
              IDBFS.storeRemoteEntry(store, path, entry, done);
            });
          }
        }
  
        // sort paths in descending order so files are deleted before their
        // parent directories
        for (var path of remove.sort().reverse()) {
          if (dst.type === 'local') {
            IDBFS.removeLocalEntry(path, done);
          } else {
            IDBFS.removeRemoteEntry(store, path, done);
          }
        }
      },
  };
  
  var asyncLoad = async (url) => {
      var arrayBuffer = await readAsync(url);
      return new Uint8Array(arrayBuffer);
    };
  
  
  var FS_createDataFile = (...args) => FS.createDataFile(...args);
  
  var getUniqueRunDependency = (id) => {
      return id;
    };
  
  var dependenciesPromise = null;
  var resolveRunDependencies = async () => dependenciesPromise;
  var runDependencies = 0;
  
  
  var dependenciesPromiseResolve = null;
  var removeRunDependency = (id) => {
      runDependencies--;
  
      Module['monitorRunDependencies']?.(runDependencies);
  
      if (!runDependencies) {
        dependenciesPromiseResolve();
      }
    };
  
  
  var addRunDependency = (id) => {
      if (!runDependencies) {
        dependenciesPromise = new Promise((resolve) => dependenciesPromiseResolve = resolve);
      }
      runDependencies++;
  
      Module['monitorRunDependencies']?.(runDependencies);
  
    };
  
  
  var preloadPlugins = [];
  var FS_handledByPreloadPlugin = async (byteArray, fullname) => {
      // Ensure plugins are ready.
      if (typeof Browser != 'undefined') Browser.init();
  
      for (var plugin of preloadPlugins) {
        if (plugin['canHandle'](fullname)) {
          return plugin['handle'](byteArray, fullname);
        }
      }
      // If no plugin handled this file then return the original/unmodified
      // byteArray.
      return byteArray;
    };
  var FS_preloadFile = async (parent, name, url, canRead, canWrite, dontCreateFile, canOwn, preFinish) => {
      // TODO we should allow people to just pass in a complete filename instead
      // of parent and name being that we just join them anyways
      var fullname = name ? PATH_FS.resolve(PATH.join2(parent, name)) : parent;
      var dep = getUniqueRunDependency(`cp ${fullname}`); // might have several active requests for the same fullname
      addRunDependency(dep);
  
      try {
        var byteArray = url;
        if (typeof url == 'string') {
          byteArray = await asyncLoad(url);
        }
  
        byteArray = await FS_handledByPreloadPlugin(byteArray, fullname);
        preFinish?.();
        if (!dontCreateFile) {
          FS_createDataFile(parent, name, byteArray, canRead, canWrite, canOwn);
        }
      } finally {
        removeRunDependency(dep);
      }
    };
  var FS_createPreloadedFile = (parent, name, url, canRead, canWrite, onload, onerror, dontCreateFile, canOwn, preFinish) => {
      FS_preloadFile(parent, name, url, canRead, canWrite, dontCreateFile, canOwn, preFinish).then(onload).catch(onerror);
    };
  
  var FS = {
  root:null,
  mounts:[],
  devices:{
  },
  streams:[],
  nextInode:1,
  nameTable:null,
  currentPath:"/",
  initialized:false,
  ignorePermissions:true,
  filesystems:null,
  syncFSRequests:0,
  ErrnoError:class {
        name = 'ErrnoError';
        // We set the `name` property to be able to identify `FS.ErrnoError`
        // - the `name` is a standard ECMA-262 property of error objects. Kind of good to have it anyway.
        // - when using PROXYFS, an error can come from an underlying FS
        // as different FS objects have their own FS.ErrnoError each,
        // the test `err instanceof FS.ErrnoError` won't detect an error coming from another filesystem, causing bugs.
        // we'll use the reliable test `err.name == "ErrnoError"` instead
        constructor(errno) {
          this.errno = errno;
        }
      },
  FSStream:class {
        shared = {};
        get object() {
          return this.node;
        }
        set object(val) {
          this.node = val;
        }
        get isRead() {
          return (this.flags & 2097155) !== 1;
        }
        get isWrite() {
          return (this.flags & 2097155) !== 0;
        }
        get isAppend() {
          return (this.flags & 1024);
        }
        get flags() {
          return this.shared.flags;
        }
        set flags(val) {
          this.shared.flags = val;
        }
        get position() {
          return this.shared.position;
        }
        set position(val) {
          this.shared.position = val;
        }
      },
  FSNode:class {
        node_ops = {};
        stream_ops = {};
        readMode = 292 | 73;
        writeMode = 146;
        mounted = null;
        constructor(parent, name, mode, rdev) {
          if (!parent) {
            parent = this;  // root node sets parent to itself
          }
          this.parent = parent;
          this.mount = parent.mount;
          this.id = FS.nextInode++;
          this.name = name;
          this.mode = mode;
          this.rdev = rdev;
          this.atime = this.mtime = this.ctime = Date.now();
        }
        get read() {
          return (this.mode & this.readMode) === this.readMode;
        }
        set read(val) {
          val ? this.mode |= this.readMode : this.mode &= ~this.readMode;
        }
        get write() {
          return (this.mode & this.writeMode) === this.writeMode;
        }
        set write(val) {
          val ? this.mode |= this.writeMode : this.mode &= ~this.writeMode;
        }
        get isFolder() {
          return FS.isDir(this.mode);
        }
        get isDevice() {
          return FS.isChrdev(this.mode);
        }
        // The per-inode readiness wait-queue. The node carries a Set of listener
        // entries {cb}; producers (SOCKFS, PIPEFS) call notifyListeners on a
        // readiness transition, and poll()/epoll consume it. It lives on the node
        // (not the fd) so dup'd fds share one queue. Only nodes that derive real
        // readiness (sockets, pipes, and an epoll's own node) ever use this -
        // always-ready types (regular files, ttys) never register or notify.
        addListener(cb, exclusive = false) {
          var entry = {cb, exclusive};
          var listeners = (this.listeners ??= new Set());
          listeners.add(entry);
          return {listeners, entry};
        }
        notifyListeners(flags) {
          // Iterates the set without copying, which is safe ONLY under a
          // load-bearing contract that every internal listener must honour:
          //   1. A listener must not run user code synchronously (a poll waiter only
          //      resolves a Promise; an epoll registration only re-lists +
          //      re-notifies; the epoll callback only schedules a tick). User code
          //      runs on a later tick, never inside this loop.
          //   2. A listener may delete entries only from ITS OWN waiter, never from
          //      a sibling node's set that may be mid-iteration. (Deleting an entry
          //      of the set being iterated here is fine - a Set tolerates removal of
          //      a not-yet-visited entry mid-iteration; mutating a *different* node's
          //      set is fine because that set is not being iterated.)
          // Violating either gives silently skipped wakeups that are near-impossible
          // to reproduce. Any new producer/listener must preserve it.
          if (!this.listeners) return;
          // Fire every non-exclusive listener. Among EPOLLEXCLUSIVE registrations
          // (one fd watched by several epolls) wake only one, rotating round-robin
          // per node, to avoid a thundering herd. (Only epoll registrations are ever
          // exclusive; poll waiters and a node's own consumers are not.)
          var excl;
          for (var entry of this.listeners) {
            if (entry.exclusive) (excl ||= []).push(entry);
            else entry.cb(flags);
          }
          if (excl) {
            var i = (this.exclTurn || 0) % excl.length;
            this.exclTurn = i + 1;
            excl[i].cb(flags);
          }
        }
      },
  lookupPath(path, opts = {}) {
        if (!path) {
          throw new FS.ErrnoError(44);
        }
        opts.follow_mount ??= true
  
        if (!PATH.isAbs(path)) {
          path = FS.cwd() + '/' + path;
        }
  
        // limit max consecutive symlinks to SYMLOOP_MAX.
        linkloop: for (var nlinks = 0; nlinks < 40; nlinks++) {
          // split the absolute path
          var parts = path.split('/').filter((p) => !!p);
  
          // start at the root
          var current = FS.root;
          var current_path = '/';
  
          for (var i = 0; i < parts.length; i++) {
            var islast = (i === parts.length-1);
            if (islast && opts.parent) {
              // stop resolving
              break;
            }
  
            if (parts[i] === '.') {
              continue;
            }
  
            if (parts[i] === '..') {
              current_path = PATH.dirname(current_path);
              if (FS.isRoot(current)) {
                path = current_path + '/' + parts.slice(i + 1).join('/');
                // We're making progress here, don't let many consecutive ..'s
                // lead to ELOOP
                nlinks--;
                continue linkloop;
              } else {
                current = current.parent;
              }
              continue;
            }
  
            current_path = PATH.join2(current_path, parts[i]);
            try {
              current = FS.lookupNode(current, parts[i]);
            } catch (e) {
              // if noent_okay is true, suppress a ENOENT in the last component
              // and return an object with an undefined node. This is needed for
              // resolving symlinks in the path when creating a file.
              if ((e?.errno === 44) && islast && opts.noent_okay) {
                return { path: current_path };
              }
              throw e;
            }
  
            // jump to the mount's root node if this is a mountpoint
            if (FS.isMountpoint(current) && (!islast || opts.follow_mount)) {
              current = current.mounted.root;
            }
  
            // by default, lookupPath will not follow a symlink if it is the final path component.
            // setting opts.follow = true will override this behavior.
            if (FS.isLink(current.mode) && (!islast || opts.follow)) {
              if (!current.node_ops.readlink) {
                throw new FS.ErrnoError(52);
              }
              var link = current.node_ops.readlink(current);
              if (!PATH.isAbs(link)) {
                link = PATH.dirname(current_path) + '/' + link;
              }
              path = link + '/' + parts.slice(i + 1).join('/');
              continue linkloop;
            }
          }
          return { path: current_path, node: current };
        }
        throw new FS.ErrnoError(32);
      },
  getPath(node) {
        var path;
        while (true) {
          if (FS.isRoot(node)) {
            var mount = node.mount.mountpoint;
            if (!path) return mount;
            return mount[mount.length-1] !== '/' ? `${mount}/${path}` : mount + path;
          }
          path = path ? `${node.name}/${path}` : node.name;
          node = node.parent;
        }
      },
  hashName(parentid, name) {
        var hash = 0;
  
        for (var i = 0; i < name.length; i++) {
          hash = ((hash << 5) - hash + name.charCodeAt(i)) | 0;
        }
        return ((parentid + hash) >>> 0) % FS.nameTable.length;
      },
  hashAddNode(node) {
        var hash = FS.hashName(node.parent.id, node.name);
        node.name_next = FS.nameTable[hash];
        FS.nameTable[hash] = node;
      },
  hashRemoveNode(node) {
        var hash = FS.hashName(node.parent.id, node.name);
        if (FS.nameTable[hash] === node) {
          FS.nameTable[hash] = node.name_next;
        } else {
          var current = FS.nameTable[hash];
          while (current) {
            if (current.name_next === node) {
              current.name_next = node.name_next;
              break;
            }
            current = current.name_next;
          }
        }
      },
  lookupNode(parent, name) {
        var errCode = FS.mayLookup(parent);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        var hash = FS.hashName(parent.id, name);
        for (var node = FS.nameTable[hash]; node; node = node.name_next) {
          var nodeName = node.name;
          if (node.parent.id === parent.id && nodeName === name) {
            return node;
          }
        }
        // if we failed to find it in the cache, call into the VFS
        return FS.lookup(parent, name);
      },
  createNode(parent, name, mode, rdev) {
        var node = new FS.FSNode(parent, name, mode, rdev);
  
        FS.hashAddNode(node);
  
        return node;
      },
  destroyNode(node) {
        FS.hashRemoveNode(node);
      },
  isRoot(node) {
        return node === node.parent;
      },
  isMountpoint(node) {
        return !!node.mounted;
      },
  isFile(mode) {
        return (mode & 61440) === 32768;
      },
  isDir(mode) {
        return (mode & 61440) === 16384;
      },
  isLink(mode) {
        return (mode & 61440) === 40960;
      },
  isChrdev(mode) {
        return (mode & 61440) === 8192;
      },
  isBlkdev(mode) {
        return (mode & 61440) === 24576;
      },
  isFIFO(mode) {
        return (mode & 61440) === 4096;
      },
  isSocket(mode) {
        return (mode & 49152) === 49152;
      },
  flagsToPermissionString(flag) {
        var perms = ['r', 'w', 'rw'][flag & 3];
        if ((flag & 512)) {
          perms += 'w';
        }
        return perms;
      },
  nodePermissions(node, perms) {
        if (FS.ignorePermissions) {
          return 0;
        }
        // return 0 if any user, group or owner bits are set.
        if (perms.includes('r') && !(node.mode & 292)) {
          return 2;
        }
        if (perms.includes('w') && !(node.mode & 146)) {
          return 2;
        }
        if (perms.includes('x') && !(node.mode & 73)) {
          return 2;
        }
        return 0;
      },
  mayLookup(dir) {
        if (!FS.isDir(dir.mode)) return 54;
        var errCode = FS.nodePermissions(dir, 'x');
        if (errCode) return errCode;
        if (!dir.node_ops.lookup) return 2;
        return 0;
      },
  mayCreate(dir, name) {
        if (!FS.isDir(dir.mode)) {
          return 54;
        }
        try {
          var node = FS.lookupNode(dir, name);
          return 20;
        } catch (e) {
        }
        return FS.nodePermissions(dir, 'wx');
      },
  mayDelete(dir, name, isdir) {
        var node;
        try {
          node = FS.lookupNode(dir, name);
        } catch (e) {
          return e.errno;
        }
        var errCode = FS.nodePermissions(dir, 'wx');
        if (errCode) {
          return errCode;
        }
        if (isdir) {
          if (!FS.isDir(node.mode)) {
            return 54;
          }
          if (FS.isRoot(node) || FS.getPath(node) === FS.cwd()) {
            return 10;
          }
        } else if (FS.isDir(node.mode)) {
          return 31;
        }
        return 0;
      },
  mayOpen(node, flags) {
        if (!node) {
          return 44;
        }
        if (FS.isLink(node.mode)) {
          return 32;
        }
        var mode = FS.flagsToPermissionString(flags);
        if (FS.isDir(node.mode)) {
          // opening for write
          // TODO: check for O_SEARCH? (== search for dir only)
          if (mode !== 'r' || (flags & (512 | 64))) {
            return 31;
          }
        }
        return FS.nodePermissions(node, mode);
      },
  checkOpExists(op, err) {
        if (!op) {
          throw new FS.ErrnoError(err);
        }
        return op;
      },
  MAX_OPEN_FDS:4096,
  nextfd() {
        for (var fd = 0; fd <= FS.MAX_OPEN_FDS; fd++) {
          if (!FS.streams[fd]) {
            return fd;
          }
        }
        throw new FS.ErrnoError(33);
      },
  getStreamChecked(fd) {
        var stream = FS.getStream(fd);
        if (!stream) {
          throw new FS.ErrnoError(8);
        }
        return stream;
      },
  getStream:(fd) => FS.streams[fd],
  createStream(stream, fd = -1) {
  
        // clone it, so we can return an instance of FSStream
        stream = Object.assign(new FS.FSStream(), stream);
        if (fd == -1) {
          fd = FS.nextfd();
        }
        stream.fd = fd;
        FS.streams[fd] = stream;
        return stream;
      },
  closeStream(fd) {
        FS.streams[fd] = null;
      },
  dupStream(origStream, fd = -1) {
        var stream = FS.createStream(origStream, fd);
        stream.stream_ops?.dup?.(stream);
        return stream;
      },
  doSetAttr(stream, node, attr) {
        var setattr = stream?.stream_ops.setattr;
        var arg = setattr ? stream : node;
        setattr ??= node.node_ops.setattr;
        FS.checkOpExists(setattr, 63)
        try {
          setattr(arg, attr);
        } catch (e) {
          if (e instanceof RangeError) {
            throw new FS.ErrnoError(22);
          }
          throw e;
        }
      },
  chrdev_stream_ops:{
  open(stream) {
          var device = FS.getDevice(stream.node.rdev);
          // override node's stream ops with the device's
          stream.stream_ops = device.stream_ops;
          // forward the open call
          stream.stream_ops.open?.(stream);
        },
  llseek() {
          throw new FS.ErrnoError(70);
        },
  },
  major:(dev) => ((dev) >> 8),
  minor:(dev) => ((dev) & 0xff),
  makedev:(ma, mi) => ((ma) << 8 | (mi)),
  registerDevice(dev, ops) {
        FS.devices[dev] = { stream_ops: ops };
      },
  getDevice:(dev) => FS.devices[dev],
  getMounts(mount) {
        var mounts = [];
        var check = [mount];
  
        while (check.length) {
          var m = check.pop();
  
          mounts.push(m);
  
          check.push(...m.mounts);
        }
  
        return mounts;
      },
  syncfs(populate, callback) {
        if (typeof populate == 'function') {
          callback = populate;
          populate = false;
        }
  
        FS.syncFSRequests++;
  
        if (FS.syncFSRequests > 1) {
          err(`warning: ${FS.syncFSRequests} FS.syncfs operations in flight at once, probably just doing extra work`);
        }
  
        var mounts = FS.getMounts(FS.root.mount);
        var completed = 0;
  
        function doCallback(errCode) {
          FS.syncFSRequests--;
          return callback(errCode);
        }
  
        function done(errCode) {
          if (errCode) {
            if (!done.errored) {
              done.errored = true;
              return doCallback(errCode);
            }
            return;
          }
          if (++completed >= mounts.length) {
            doCallback(null);
          }
        };
  
        // sync all mounts
        for (var mount of mounts) {
          if (mount.type.syncfs) {
            mount.type.syncfs(mount, populate, done);
          } else {
            done(null);
          }
        }
      },
  mount(type, opts, mountpoint) {
        var root = mountpoint === '/';
        var pseudo = !mountpoint;
        var node;
  
        if (root && FS.root) {
          throw new FS.ErrnoError(10);
        } else if (!root && !pseudo) {
          var lookup = FS.lookupPath(mountpoint, { follow_mount: false });
  
          mountpoint = lookup.path;  // use the absolute path
          node = lookup.node;
  
          if (FS.isMountpoint(node)) {
            throw new FS.ErrnoError(10);
          }
  
          if (!FS.isDir(node.mode)) {
            throw new FS.ErrnoError(54);
          }
        }
  
        var mount = {
          type,
          opts,
          mountpoint,
          mounts: []
        };
  
        // create a root node for the fs
        var mountRoot = type.mount(mount);
        mountRoot.mount = mount;
        mount.root = mountRoot;
  
        if (root) {
          FS.root = mountRoot;
        } else if (node) {
          // set as a mountpoint
          node.mounted = mount;
  
          // add the new mount to the current mount's children
          if (node.mount) {
            node.mount.mounts.push(mount);
          }
        }
  
        return mountRoot;
      },
  unmount(mountpoint) {
        var lookup = FS.lookupPath(mountpoint, { follow_mount: false });
  
        if (!FS.isMountpoint(lookup.node)) {
          throw new FS.ErrnoError(28);
        }
  
        // destroy the nodes for this mount, and all its child mounts
        var node = lookup.node;
        var mount = node.mounted;
        var mounts = FS.getMounts(mount);
  
        for (var [hash, current] of Object.entries(FS.nameTable)) {
          while (current) {
            var next = current.name_next;
  
            if (mounts.includes(current.mount)) {
              FS.destroyNode(current);
            }
  
            current = next;
          }
        }
  
        // no longer a mountpoint
        node.mounted = null;
  
        // remove this mount from the child mounts
        var idx = node.mount.mounts.indexOf(mount);
        node.mount.mounts.splice(idx, 1);
      },
  lookup(parent, name) {
        return parent.node_ops.lookup(parent, name);
      },
  mknod(path, mode, dev) {
        var lookup = FS.lookupPath(path, { parent: true });
        var parent = lookup.node;
        var name = PATH.basename(path);
        if (!name) {
          throw new FS.ErrnoError(28);
        }
        if (name === '.' || name === '..') {
          throw new FS.ErrnoError(20);
        }
        var errCode = FS.mayCreate(parent, name);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        if (!parent.node_ops.mknod) {
          throw new FS.ErrnoError(63);
        }
        return parent.node_ops.mknod(parent, name, mode, dev);
      },
  statfs(path) {
        return FS.statfsNode(FS.lookupPath(path, {follow: true}).node);
      },
  statfsStream(stream) {
        // We keep a separate statfsStream function because noderawfs overrides
        // it. In noderawfs, stream.node is sometimes null. Instead, we need to
        // look at stream.path.
        return FS.statfsNode(stream.node);
      },
  statfsNode(node) {
        // NOTE: None of the defaults here are true. We're just returning safe and
        //       sane values. Currently nodefs and rawfs replace these defaults,
        //       other file systems leave them alone.
        var rtn = {
          bsize: 4096,
          frsize: 4096,
          blocks: 1e6,
          bfree: 5e5,
          bavail: 5e5,
          files: FS.nextInode,
          ffree: FS.nextInode - 1,
          fsid: 42,
          flags: 2,
          namelen: 255,
        };
  
        if (node.node_ops.statfs) {
          Object.assign(rtn, node.node_ops.statfs(node.mount.opts.root));
        }
        return rtn;
      },
  create(path, mode = 0o666) {
        mode &= 4095;
        mode |= 32768;
        return FS.mknod(path, mode, 0);
      },
  mkdir(path, mode = 0o777) {
        mode &= 511 | 512;
        mode |= 16384;
        return FS.mknod(path, mode, 0);
      },
  mkdirTree(path, mode) {
        var dirs = path.split('/');
        var d = '';
        for (var dir of dirs) {
          if (!dir) continue;
          if (d || PATH.isAbs(path)) d += '/';
          d += dir;
          try {
            FS.mkdir(d, mode);
          } catch(e) {
            if (e.errno != 20) throw e;
          }
        }
      },
  mkdev(path, mode, dev) {
        if (typeof dev == 'undefined') {
          dev = mode;
          mode = 0o666;
        }
        mode |= 8192;
        return FS.mknod(path, mode, dev);
      },
  symlink(oldpath, newpath) {
        if (!PATH_FS.resolve(oldpath)) {
          throw new FS.ErrnoError(44);
        }
        var lookup = FS.lookupPath(newpath, { parent: true });
        var parent = lookup.node;
        if (!parent) {
          throw new FS.ErrnoError(44);
        }
        var newname = PATH.basename(newpath);
        var errCode = FS.mayCreate(parent, newname);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        if (!parent.node_ops.symlink) {
          throw new FS.ErrnoError(63);
        }
        return parent.node_ops.symlink(parent, newname, oldpath);
      },
  link(oldpath, newpath, flags) {
        var lookup = FS.lookupPath(newpath, { parent: true });
        var parent = lookup.node;
        if (!parent) {
          throw new FS.ErrnoError(44);
        }
        var newname = PATH.basename(newpath);
        var errCode = FS.mayCreate(parent, newname);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        // Hardlinks are only supported by filesystem backends that provide a
        // `link` node op (e.g. NODERAWFS backed by the host). NODEFS omits it:
        // a host hardlink cannot be confined to the mount root.
        if (!parent.node_ops.link) {
          throw new FS.ErrnoError(34);
        }
        return parent.node_ops.link(parent, newname, oldpath, flags);
      },
  rename(old_path, new_path) {
        var old_dirname = PATH.dirname(old_path);
        var new_dirname = PATH.dirname(new_path);
        var old_name = PATH.basename(old_path);
        var new_name = PATH.basename(new_path);
        // parents must exist
        var lookup, old_dir, new_dir;
  
        // let the errors from non existent directories percolate up
        lookup = FS.lookupPath(old_path, { parent: true });
        old_dir = lookup.node;
        lookup = FS.lookupPath(new_path, { parent: true });
        new_dir = lookup.node;
  
        if (!old_dir || !new_dir) throw new FS.ErrnoError(44);
        // need to be part of the same mount
        if (old_dir.mount !== new_dir.mount) {
          throw new FS.ErrnoError(75);
        }
        // source must exist
        var old_node = FS.lookupNode(old_dir, old_name);
        // old path should not be an ancestor of the new path
        var relative = PATH_FS.relative(old_path, new_dirname);
        if (relative.charAt(0) !== '.') {
          throw new FS.ErrnoError(28);
        }
        // new path should not be an ancestor of the old path
        relative = PATH_FS.relative(new_path, old_dirname);
        if (relative.charAt(0) !== '.') {
          throw new FS.ErrnoError(55);
        }
        // see if the new path already exists
        var new_node;
        try {
          new_node = FS.lookupNode(new_dir, new_name);
        } catch (e) {
          // not fatal
        }
        // early out if nothing needs to change
        if (old_node === new_node) {
          return;
        }
        // we'll need to delete the old entry
        var isdir = FS.isDir(old_node.mode);
        var errCode = FS.mayDelete(old_dir, old_name, isdir);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        // need delete permissions if we'll be overwriting.
        // need create permissions if new doesn't already exist.
        errCode = new_node ?
          FS.mayDelete(new_dir, new_name, isdir) :
          FS.mayCreate(new_dir, new_name);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        if (!old_dir.node_ops.rename) {
          throw new FS.ErrnoError(63);
        }
        if (FS.isMountpoint(old_node) || (new_node && FS.isMountpoint(new_node))) {
          throw new FS.ErrnoError(10);
        }
        // if we are going to change the parent, check write permissions
        if (new_dir !== old_dir) {
          errCode = FS.nodePermissions(old_dir, 'w');
          if (errCode) {
            throw new FS.ErrnoError(errCode);
          }
        }
        // remove the node from the lookup hash
        FS.hashRemoveNode(old_node);
        // do the underlying fs rename
        try {
          old_dir.node_ops.rename(old_node, new_dir, new_name);
          // update old node (we do this here to avoid each backend
          // needing to)
          old_node.parent = new_dir;
        } catch (e) {
          throw e;
        } finally {
          // add the node back to the hash (in case node_ops.rename
          // changed its name)
          FS.hashAddNode(old_node);
        }
      },
  rmdir(path) {
        var lookup = FS.lookupPath(path, { parent: true });
        var parent = lookup.node;
        var name = PATH.basename(path);
        var node = FS.lookupNode(parent, name);
        var errCode = FS.mayDelete(parent, name, true);
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        if (!parent.node_ops.rmdir) {
          throw new FS.ErrnoError(63);
        }
        if (FS.isMountpoint(node)) {
          throw new FS.ErrnoError(10);
        }
        parent.node_ops.rmdir(parent, name);
        FS.destroyNode(node);
      },
  readdir(path) {
        var lookup = FS.lookupPath(path, { follow: true });
        var node = lookup.node;
        var readdir = FS.checkOpExists(node.node_ops.readdir, 54);
        return readdir(node);
      },
  unlink(path) {
        var lookup = FS.lookupPath(path, { parent: true });
        var parent = lookup.node;
        if (!parent) {
          throw new FS.ErrnoError(44);
        }
        var name = PATH.basename(path);
        var node = FS.lookupNode(parent, name);
        var errCode = FS.mayDelete(parent, name, false);
        if (errCode) {
          // According to POSIX, we should map EISDIR to EPERM, but
          // we instead do what Linux does (and we must, as we use
          // the musl linux libc).
          throw new FS.ErrnoError(errCode);
        }
        if (!parent.node_ops.unlink) {
          throw new FS.ErrnoError(63);
        }
        if (FS.isMountpoint(node)) {
          throw new FS.ErrnoError(10);
        }
        parent.node_ops.unlink(parent, name);
        FS.destroyNode(node);
      },
  readlink(path) {
        var lookup = FS.lookupPath(path);
        var link = lookup.node;
        if (!link) {
          throw new FS.ErrnoError(44);
        }
        if (!link.node_ops.readlink) {
          throw new FS.ErrnoError(28);
        }
        return link.node_ops.readlink(link);
      },
  stat(path, dontFollow) {
        var lookup = FS.lookupPath(path, { follow: !dontFollow });
        var node = lookup.node;
        var getattr = FS.checkOpExists(node.node_ops.getattr, 63);
        return getattr(node);
      },
  fstat(fd) {
        var stream = FS.getStreamChecked(fd);
        var node = stream.node;
        var getattr = stream.stream_ops.getattr;
        var arg = getattr ? stream : node;
        getattr ??= node.node_ops.getattr;
        FS.checkOpExists(getattr, 63)
        return getattr(arg);
      },
  lstat(path) {
        return FS.stat(path, true);
      },
  doChmod(stream, node, mode, dontFollow) {
        FS.doSetAttr(stream, node, {
          mode: (mode & 4095) | (node.mode & ~4095),
          ctime: Date.now(),
          dontFollow
        });
      },
  chmod(path, mode, dontFollow) {
        var node;
        if (typeof path == 'string') {
          var lookup = FS.lookupPath(path, { follow: !dontFollow });
          node = lookup.node;
        } else {
          node = path;
        }
        FS.doChmod(null, node, mode, dontFollow);
      },
  lchmod(path, mode) {
        FS.chmod(path, mode, true);
      },
  fchmod(fd, mode) {
        var stream = FS.getStreamChecked(fd);
        FS.doChmod(stream, stream.node, mode, false);
      },
  doChown(stream, node, dontFollow) {
        FS.doSetAttr(stream, node, {
          timestamp: Date.now(),
          dontFollow
          // we ignore the uid / gid for now
        });
      },
  chown(path, uid, gid, dontFollow) {
        var node;
        if (typeof path == 'string') {
          var lookup = FS.lookupPath(path, { follow: !dontFollow });
          node = lookup.node;
        } else {
          node = path;
        }
        FS.doChown(null, node, dontFollow);
      },
  lchown(path, uid, gid) {
        FS.chown(path, uid, gid, true);
      },
  fchown(fd, uid, gid) {
        var stream = FS.getStreamChecked(fd);
        FS.doChown(stream, stream.node, false);
      },
  doTruncate(stream, node, len) {
        if (FS.isDir(node.mode)) {
          throw new FS.ErrnoError(31);
        }
        if (!FS.isFile(node.mode)) {
          throw new FS.ErrnoError(28);
        }
        var errCode = FS.nodePermissions(node, 'w');
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        FS.doSetAttr(stream, node, {
          size: len,
          timestamp: Date.now()
        });
      },
  truncate(path, len) {
        if (len < 0) {
          throw new FS.ErrnoError(28);
        }
        var node;
        if (typeof path == 'string') {
          var lookup = FS.lookupPath(path, { follow: true });
          node = lookup.node;
        } else {
          node = path;
        }
        FS.doTruncate(null, node, len);
      },
  ftruncate(fd, len) {
        var stream = FS.getStreamChecked(fd);
        if (len < 0 || (stream.flags & 2097155) === 0) {
          throw new FS.ErrnoError(28);
        }
        FS.doTruncate(stream, stream.node, len);
      },
  utime(path, atime, mtime, dontFollow) {
        var lookup = FS.lookupPath(path, { follow: !dontFollow });
        FS.doSetAttr(null, lookup.node, {
          atime: atime,
          mtime: mtime,
          dontFollow
        });
      },
  open(path, flags, mode = 0o666) {
        if (path === '') {
          throw new FS.ErrnoError(44);
        }
        flags = FS_modeStringToFlags(flags);
        if ((flags & 64)) {
          mode = (mode & 4095) | 32768;
        } else {
          mode = 0;
        }
        var node;
        var isDirPath;
        if (typeof path == 'object') {
          node = path;
        } else {
          isDirPath = path.endsWith('/');
          // noent_okay makes it so that if the final component of the path
          // doesn't exist, lookupPath returns `node: undefined`. `path` will be
          // updated to point to the target of all symlinks.
          var lookup = FS.lookupPath(path, {
            follow: !(flags & 131072),
            noent_okay: true
          });
          node = lookup.node;
          path = lookup.path;
        }
        // perhaps we need to create the node
        var created = false;
        if ((flags & 64)) {
          if (node) {
            // if O_CREAT and O_EXCL are set, error out if the node already exists
            if ((flags & 128)) {
              throw new FS.ErrnoError(20);
            }
          } else if (isDirPath) {
            throw new FS.ErrnoError(31);
          } else {
            // node doesn't exist, try to create it
            // Ignore the permission bits here to ensure we can `open` this new
            // file below. We use chmod below to apply the permissions once the
            // file is open.
            node = FS.mknod(path, mode | 0o777, 0);
            created = true;
          }
        }
        if (!node) {
          throw new FS.ErrnoError(44);
        }
        // can't truncate a device
        if (FS.isChrdev(node.mode)) {
          flags &= ~512;
        }
        // if asked only for a directory, then this must be one
        if ((flags & 65536) && !FS.isDir(node.mode)) {
          throw new FS.ErrnoError(54);
        }
        // check permissions, if this is not a file we just created now (it is ok to
        // create and write to a file with read-only permissions; it is read-only
        // for later use)
        if (!created) {
          var errCode = FS.mayOpen(node, flags);
          if (errCode) {
            throw new FS.ErrnoError(errCode);
          }
        }
        // do truncation if necessary
        if ((flags & 512) && !created) {
          FS.truncate(node, 0);
        }
        // we've already handled these, don't pass down to the underlying vfs
        flags &= ~(128 | 512 | 131072);
  
        // register the stream with the filesystem
        var stream = FS.createStream({
          node,
          path: FS.getPath(node),  // we want the absolute path to the node
          flags,
          seekable: true,
          position: 0,
          stream_ops: node.stream_ops,
          // used by the file family libc calls (fopen, fwrite, ferror, etc.)
          ungotten: [],
          error: false
        });
        // call the new stream's open function
        if (stream.stream_ops.open) {
          stream.stream_ops.open(stream);
        }
        if (created) {
          FS.chmod(node, mode & 0o777);
        }
        return stream;
      },
  close(stream) {
        if (FS.isClosed(stream)) {
          throw new FS.ErrnoError(8);
        }
        if (stream.getdents) stream.getdents = null; // free readdir state
        // The fd is going away: wake anything waiting on it (poll/epoll) with
        // POLLNVAL so a blocking wait unblocks and an epoll registration is evicted
        // on its next derive. Only sockets/pipes/epoll ever carry a wait-queue, so
        // for every other stream (incl. nodeless noderawfs stdio) this is a no-op.
        stream.node?.notifyListeners(32);
        try {
          if (stream.stream_ops.close) {
            stream.stream_ops.close(stream);
          }
        } catch (e) {
          throw e;
        } finally {
          FS.closeStream(stream.fd);
        }
        stream.fd = null;
      },
  isClosed(stream) {
        return stream.fd === null;
      },
  llseek(stream, offset, whence) {
        if (FS.isClosed(stream)) {
          throw new FS.ErrnoError(8);
        }
        if (!stream.seekable || !stream.stream_ops.llseek) {
          throw new FS.ErrnoError(70);
        }
        if (whence != 0 && whence != 1 && whence != 2) {
          throw new FS.ErrnoError(28);
        }
        stream.position = stream.stream_ops.llseek(stream, offset, whence);
        stream.ungotten = [];
        return stream.position;
      },
  read(stream, buffer, offset, length, position) {
        if (length < 0 || position < 0) {
          throw new FS.ErrnoError(28);
        }
        if (FS.isClosed(stream)) {
          throw new FS.ErrnoError(8);
        }
        if ((stream.flags & 2097155) === 1) {
          throw new FS.ErrnoError(8);
        }
        if (FS.isDir(stream.node.mode)) {
          throw new FS.ErrnoError(31);
        }
        if (!stream.stream_ops.read) {
          throw new FS.ErrnoError(28);
        }
        var seeking = typeof position != 'undefined';
        if (!seeking) {
          position = stream.position;
        } else if (!stream.seekable) {
          throw new FS.ErrnoError(70);
        }
        var bytesRead = stream.stream_ops.read(stream, buffer, offset, length, position);
        if (!seeking) stream.position += bytesRead;
        return bytesRead;
      },
  write(stream, buffer, offset, length, position, canOwn) {
        if (length < 0 || position < 0) {
          throw new FS.ErrnoError(28);
        }
        if (FS.isClosed(stream)) {
          throw new FS.ErrnoError(8);
        }
        if ((stream.flags & 2097155) === 0) {
          throw new FS.ErrnoError(8);
        }
        if (FS.isDir(stream.node.mode)) {
          throw new FS.ErrnoError(31);
        }
        if (!stream.stream_ops.write) {
          throw new FS.ErrnoError(28);
        }
        if (stream.seekable && stream.flags & 1024) {
          // seek to the end before writing in append mode
          FS.llseek(stream, 0, 2);
        }
        var seeking = typeof position != 'undefined';
        if (!seeking) {
          position = stream.position;
        } else if (!stream.seekable) {
          throw new FS.ErrnoError(70);
        }
        var bytesWritten = stream.stream_ops.write(stream, buffer, offset, length, position, canOwn);
        if (!seeking) stream.position += bytesWritten;
        return bytesWritten;
      },
  mmap(stream, length, position, prot, flags) {
        // User requests writing to file (prot & PROT_WRITE != 0).
        // Checking if we have permissions to write to the file unless
        // MAP_PRIVATE flag is set. According to POSIX spec it is possible
        // to write to file opened in read-only mode with MAP_PRIVATE flag,
        // as all modifications will be visible only in the memory of
        // the current process.
        if ((prot & 2)
            && !(flags & 2)
            && (stream.flags & 2097155) !== 2) {
          throw new FS.ErrnoError(2);
        }
        if ((stream.flags & 2097155) === 1) {
          throw new FS.ErrnoError(2);
        }
        if (!stream.stream_ops.mmap) {
          throw new FS.ErrnoError(43);
        }
        if (!length) {
          throw new FS.ErrnoError(28);
        }
        return stream.stream_ops.mmap(stream, length, position, prot, flags);
      },
  msync(stream, buffer, offset, length, mmapFlags) {
        if (!stream.stream_ops.msync) {
          return 0;
        }
        return stream.stream_ops.msync(stream, buffer, offset, length, mmapFlags);
      },
  ioctl(stream, cmd, arg) {
        if (!stream.stream_ops.ioctl) {
          throw new FS.ErrnoError(59);
        }
        return stream.stream_ops.ioctl(stream, cmd, arg);
      },
  readFile(path, opts = {}) {
        opts.flags = opts.flags ?? 0;
        opts.encoding = opts.encoding ?? 'binary';
        if (opts.encoding !== 'utf8' && opts.encoding !== 'binary') {
          abort(`Invalid encoding type "${opts.encoding}"`);
        }
        var stream = FS.open(path, opts.flags);
        var stat = FS.stat(path);
        var length = stat.size;
        var buf = new Uint8Array(length);
        FS.read(stream, buf, 0, length, 0);
        if (opts.encoding === 'utf8') {
          buf = UTF8ArrayToString(buf);
        }
        FS.close(stream);
        return buf;
      },
  writeFile(path, data, opts = {}) {
        opts.flags = opts.flags ?? 577;
        var stream = FS.open(path, opts.flags, opts.mode);
        data = FS_fileDataToTypedArray(data);
        FS.write(stream, data, 0, data.byteLength, undefined, opts.canOwn);
        FS.close(stream);
      },
  cwd:() => FS.currentPath,
  chdir(path) {
        var lookup = FS.lookupPath(path, { follow: true });
        if (lookup.node === null) {
          throw new FS.ErrnoError(44);
        }
        if (!FS.isDir(lookup.node.mode)) {
          throw new FS.ErrnoError(54);
        }
        var errCode = FS.nodePermissions(lookup.node, 'x');
        if (errCode) {
          throw new FS.ErrnoError(errCode);
        }
        FS.currentPath = lookup.path;
      },
  createDefaultDirectories() {
        FS.mkdir('/tmp');
        FS.mkdir('/home');
        FS.mkdir('/home/web_user');
      },
  createDefaultDevices() {
        // create /dev
        FS.mkdir('/dev');
        // setup /dev/null
        FS.registerDevice(FS.makedev(1, 3), {
          read: () => 0,
          write: (stream, buffer, offset, length, pos) => length,
          llseek: () => 0,
        });
        FS.mkdev('/dev/null', FS.makedev(1, 3));
        // setup /dev/tty and /dev/tty1
        // stderr needs to print output using err() rather than out()
        // so we register a second tty just for it.
        TTY.register(FS.makedev(5, 0), TTY.default_tty_ops);
        TTY.register(FS.makedev(6, 0), TTY.default_tty1_ops);
        FS.mkdev('/dev/tty', FS.makedev(5, 0));
        FS.mkdev('/dev/tty1', FS.makedev(6, 0));
        // setup /dev/[u]random
        // use a buffer to avoid overhead of individual crypto calls per byte
        var randomBuffer = new Uint8Array(1024), randomLeft = 0;
        var randomByte = () => {
          if (!randomLeft) {
            randomFill(randomBuffer);
            randomLeft = randomBuffer.byteLength;
          }
          return randomBuffer[--randomLeft];
        };
        FS.createDevice('/dev', 'random', randomByte);
        FS.createDevice('/dev', 'urandom', randomByte);
        // we're not going to emulate the actual shm device,
        // just create the tmp dirs that reside in it commonly
        FS.mkdir('/dev/shm');
        FS.mkdir('/dev/shm/tmp');
      },
  createSpecialDirectories() {
        // create /proc/self/fd which allows /proc/self/fd/6 => readlink gives the
        // name of the stream for fd 6 (see test_unistd_ttyname)
        FS.mkdir('/proc');
        var proc_self = FS.mkdir('/proc/self');
        FS.mkdir('/proc/self/fd');
        FS.mount({
          mount() {
            var node = FS.createNode(proc_self, 'fd', 16895, 73);
            node.stream_ops = {
              llseek: MEMFS.stream_ops.llseek,
            };
            node.node_ops = {
              lookup(parent, name) {
                var fd = +name;
                var stream = FS.getStreamChecked(fd);
                var ret = {
                  parent: null,
                  mount: { mountpoint: 'fake' },
                  node_ops: { readlink: () => stream.path },
                  id: fd + 1,
                };
                ret.parent = ret; // make it look like a simple root node
                return ret;
              },
              readdir() {
                return Array.from(FS.streams.entries())
                  .filter(([k, v]) => v)
                  .map(([k, v]) => k.toString());
              }
            };
            return node;
          }
        }, {}, '/proc/self/fd');
      },
  createStandardStreams(input, output, error) {
        // TODO deprecate the old functionality of a single
        // input / output callback and that utilizes FS.createDevice
        // and instead require a unique set of stream ops
  
        // by default, we symlink the standard streams to the
        // default tty devices. however, if the standard streams
        // have been overwritten we create a unique device for
        // them instead.
        if (input) {
          FS.createDevice('/dev', 'stdin', input);
        } else {
          FS.symlink('/dev/tty', '/dev/stdin');
        }
        if (output) {
          FS.createDevice('/dev', 'stdout', null, output);
        } else {
          FS.symlink('/dev/tty', '/dev/stdout');
        }
        if (error) {
          FS.createDevice('/dev', 'stderr', null, error);
        } else {
          FS.symlink('/dev/tty1', '/dev/stderr');
        }
  
        // open default streams for the stdin, stdout and stderr devices
        var stdin = FS.open('/dev/stdin', 0);
        var stdout = FS.open('/dev/stdout', 1);
        var stderr = FS.open('/dev/stderr', 1);
      },
  staticInit() {
        FS.nameTable = new Array(4096);
  
        FS.mount(MEMFS, {}, '/');
  
        FS.createDefaultDirectories();
        FS.createDefaultDevices();
        FS.createSpecialDirectories();
  
        FS.filesystems = {
          'MEMFS': MEMFS,
          'IDBFS': IDBFS,
        };
      },
  init(input, output, error) {
        FS.initialized = true;
  
        // Allow Module.stdin etc. to provide defaults, if none explicitly passed to us here
        input ??= Module['stdin'];
        output ??= Module['stdout'];
        error ??= Module['stderr'];
  
        FS.createStandardStreams(input, output, error);
      },
  quit() {
        FS.initialized = false;
        // force-flush all streams, so we get musl std streams printed out
        // close all of our streams
        for (var stream of FS.streams) {
          if (stream) {
            FS.close(stream);
          }
        }
      },
  findObject(path, dontResolveLastLink) {
        var ret = FS.analyzePath(path, dontResolveLastLink);
        if (!ret.exists) {
          return null;
        }
        return ret.object;
      },
  analyzePath(path, dontResolveLastLink) {
        // operate from within the context of the symlink's target
        try {
          var lookup = FS.lookupPath(path, { follow: !dontResolveLastLink });
          path = lookup.path;
        } catch (e) {
        }
        var ret = {
          isRoot: false, exists: false, error: 0, name: null, path: null, object: null,
          parentExists: false, parentPath: null, parentObject: null
        };
        try {
          var lookup = FS.lookupPath(path, { parent: true });
          ret.parentExists = true;
          ret.parentPath = lookup.path;
          ret.parentObject = lookup.node;
          ret.name = PATH.basename(path);
          lookup = FS.lookupPath(path, { follow: !dontResolveLastLink });
          ret.exists = true;
          ret.path = lookup.path;
          ret.object = lookup.node;
          ret.name = lookup.node.name;
          ret.isRoot = lookup.path === '/';
        } catch (e) {
          ret.error = e.errno;
        };
        return ret;
      },
  createPath(parent, path, canRead, canWrite) {
        parent = typeof parent == 'string' ? parent : FS.getPath(parent);
        var parts = path.split('/').reverse();
        while (parts.length) {
          var part = parts.pop();
          if (!part) continue;
          var current = PATH.join2(parent, part);
          try {
            FS.mkdir(current);
          } catch (e) {
            if (e.errno != 20) throw e;
          }
          parent = current;
        }
        return current;
      },
  createFile(parent, name, properties, canRead, canWrite) {
        var path = PATH.join2(typeof parent == 'string' ? parent : FS.getPath(parent), name);
        var mode = FS_getMode(canRead, canWrite);
        return FS.create(path, mode);
      },
  createDataFile(parent, name, data, canRead, canWrite, canOwn) {
        var path = name;
        if (parent) {
          parent = typeof parent == 'string' ? parent : FS.getPath(parent);
          path = name ? PATH.join2(parent, name) : parent;
        }
        var mode = FS_getMode(canRead, canWrite);
        var node = FS.create(path, mode);
        if (data) {
          data = FS_fileDataToTypedArray(data);
          // make sure we can write to the file
          FS.chmod(node, mode | 146);
          var stream = FS.open(node, 577);
          FS.write(stream, data, 0, data.length, 0, canOwn);
          FS.close(stream);
          FS.chmod(node, mode);
        }
      },
  createDevice(parent, name, input, output) {
        var path = PATH.join2(typeof parent == 'string' ? parent : FS.getPath(parent), name);
        var mode = FS_getMode(!!input, !!output);
        FS.createDevice.major ??= 64;
        var dev = FS.makedev(FS.createDevice.major++, 0);
        // Create a fake device that a set of stream ops to emulate
        // the old behavior.
        FS.registerDevice(dev, {
          open(stream) {
            stream.seekable = false;
          },
          close(stream) {
            // flush any pending line data
            if (output?.buffer?.length) {
              output(10);
            }
          },
          read(stream, buffer, offset, length, pos /* ignored */) {
            var bytesRead = 0;
            for (var i = 0; i < length; i++) {
              var result;
              try {
                result = input();
              } catch (e) {
                throw new FS.ErrnoError(29);
              }
              if (result === undefined && !bytesRead) {
                throw new FS.ErrnoError(6);
              }
              if (result === null || result === undefined) break;
              bytesRead++;
              buffer[offset+i] = result;
            }
            if (bytesRead) {
              stream.node.atime = Date.now();
            }
            return bytesRead;
          },
          write(stream, buffer, offset, length, pos) {
            for (var i = 0; i < length; i++) {
              try {
                output(buffer[offset+i]);
              } catch (e) {
                throw new FS.ErrnoError(29);
              }
            }
            if (length) {
              stream.node.mtime = stream.node.ctime = Date.now();
            }
            return i;
          }
        });
        return FS.mkdev(path, mode, dev);
      },
  forceLoadFile(obj) {
        if (obj.isDevice || obj.isFolder || obj.link || obj.contents) return true;
        if (globalThis.XMLHttpRequest) {
          abort('Lazy loading should have been performed (contents set) in createLazyFile, but it was not. Lazy loading only works in web workers. Use --embed-file or --preload-file in emcc on the main thread.');
        } else { // Command-line.
          try {
            obj.contents = readBinary(obj.url);
          } catch (e) {
            throw new FS.ErrnoError(29);
          }
        }
      },
  createLazyFile(parent, name, url, canRead, canWrite) {
        // Lazy chunked Uint8Array (implements get and length from Uint8Array).
        // Actual getting is abstracted away for eventual reuse.
        class LazyUint8Array {
          lengthKnown = false;
          chunks = []; // Loaded chunks. Index is the chunk number
          get(idx) {
            if (idx > this.length-1 || idx < 0) {
              return undefined;
            }
            var chunkOffset = idx % this.chunkSize;
            var chunkNum = (idx / this.chunkSize)|0;
            return this.getter(chunkNum)[chunkOffset];
          }
          setDataGetter(getter) {
            this.getter = getter;
          }
          cacheLength() {
            // Find length
            var xhr = new XMLHttpRequest();
            xhr.open('HEAD', url, false);
            xhr.send(null);
            if (!(xhr.status >= 200 && xhr.status < 300 || xhr.status === 304)) abort(`Couldn't load ${url}. Status: ${xhr.status}`);
            var datalength = Number(xhr.getResponseHeader('Content-length'));
            var header;
            var hasByteServing = (header = xhr.getResponseHeader('Accept-Ranges')) && header === 'bytes';
            var usesGzip = (header = xhr.getResponseHeader('Content-Encoding')) && header === 'gzip';
  
            var chunkSize = 1024*1024; // Chunk size in bytes
  
            if (!hasByteServing) chunkSize = datalength;
  
            // Function to get a range from the remote URL.
            var doXHR = (from, to) => {
              if (from > to) abort(`invalid range (${from}, ${to}) or no bytes requested!`);
              if (to > datalength-1) abort(`only ${datalength} bytes available! programmer error!`);
  
              // TODO: Use mozResponseArrayBuffer, responseStream, etc. if available.
              var xhr = new XMLHttpRequest();
              xhr.open('GET', url, false);
              if (datalength !== chunkSize) xhr.setRequestHeader('Range', `bytes=${from}-${to}`);
  
              // Some hints to the browser that we want binary data.
              xhr.responseType = 'arraybuffer';
              if (xhr.overrideMimeType) {
                xhr.overrideMimeType('text/plain; charset=x-user-defined');
              }
  
              xhr.send(null);
              if (!(xhr.status >= 200 && xhr.status < 300 || xhr.status === 304)) abort(`Couldn't load ${url}. Status: ${xhr.status}`);
              if (xhr.response !== undefined) {
                return new Uint8Array(/** @type{Array<number>} */(xhr.response || []));
              }
              return intArrayFromString(xhr.responseText ?? '', true);
            };
            var lazyArray = this;
            lazyArray.setDataGetter((chunkNum) => {
              var start = chunkNum * chunkSize;
              var end = (chunkNum+1) * chunkSize - 1; // including this byte
              end = Math.min(end, datalength-1); // if datalength-1 is selected, this is the last block
              if (typeof lazyArray.chunks[chunkNum] == 'undefined') {
                lazyArray.chunks[chunkNum] = doXHR(start, end);
              }
              if (typeof lazyArray.chunks[chunkNum] == 'undefined') abort('doXHR failed!');
              return lazyArray.chunks[chunkNum];
            });
  
            if (usesGzip || !datalength) {
              // if the server uses gzip or doesn't supply the length, we have to download the whole file to get the (uncompressed) length
              chunkSize = datalength = 1; // this will force getter(0)/doXHR do download the whole file
              datalength = this.getter(0).length;
              chunkSize = datalength;
              out('LazyFiles on gzip forces download of the whole file when length is accessed');
            }
  
            this._length = datalength;
            this._chunkSize = chunkSize;
            this.lengthKnown = true;
          }
          get length() {
            if (!this.lengthKnown) {
              this.cacheLength();
            }
            return this._length;
          }
          get chunkSize() {
            if (!this.lengthKnown) {
              this.cacheLength();
            }
            return this._chunkSize;
          }
        }
  
        if (globalThis.XMLHttpRequest) {
          if (!ENVIRONMENT_IS_WORKER) abort('Cannot do synchronous binary XHRs outside webworkers in modern browsers. Use --embed-file or --preload-file in emcc');
          var lazyArray = new LazyUint8Array();
          var properties = { isDevice: false, contents: lazyArray };
        } else {
          var properties = { isDevice: false, url: url };
        }
  
        var node = FS.createFile(parent, name, properties, canRead, canWrite);
        // This is a total hack, but I want to get this lazy file code out of the
        // core of MEMFS. If we want to keep this lazy file concept I feel it should
        // be its own thin LAZYFS proxying calls to MEMFS.
        if (properties.contents) {
          node.contents = properties.contents;
        } else if (properties.url) {
          node.contents = null;
          node.url = properties.url;
        }
        // Add a function that defers querying the file size until it is asked the first time.
        Object.defineProperties(node, {
          usedBytes: {
            get: function() { return this.contents.length; }
          }
        });
        // override each stream op with one that tries to force load the lazy file first
        var stream_ops = {};
        for (const [key, fn] of Object.entries(node.stream_ops)) {
          stream_ops[key] = (...args) => {
            FS.forceLoadFile(node);
            return fn(...args);
          };
        }
        function writeChunks(stream, buffer, offset, length, position) {
          var contents = stream.node.contents;
          if (position >= contents.length)
            return 0;
          var size = Math.min(contents.length - position, length);
          if (contents.slice) { // normal array
            for (var i = 0; i < size; i++) {
              buffer[offset + i] = contents[position + i];
            }
          } else {
            for (var i = 0; i < size; i++) { // LazyUint8Array from sync binary XHR
              buffer[offset + i] = contents.get(position + i);
            }
          }
          return size;
        }
        // use a custom read function
        stream_ops.read = (stream, buffer, offset, length, position) => {
          FS.forceLoadFile(node);
          return writeChunks(stream, buffer, offset, length, position)
        };
        // use a custom mmap function
        stream_ops.mmap = (stream, length, position, prot, flags) => {
          FS.forceLoadFile(node);
          var ptr = mmapAlloc(length);
          if (!ptr) {
            throw new FS.ErrnoError(48);
          }
          writeChunks(stream, HEAP8, ptr, length, position);
          return { ptr, allocated: true };
        };
        node.stream_ops = stream_ops;
        return node;
      },
  };
  
  
  /** @type {!Uint8Array} */
  var HEAPU8;
  
    /**
   * Given a pointer 'ptr' to a null-terminated UTF8-encoded string in the
   * emscripten HEAP, returns a copy of that string as a Javascript String object.
   *
   * @param {number} ptr
   * @param {number=} maxBytesToRead - An optional length that specifies the
   *   maximum number of bytes to read. You can omit this parameter to scan the
   *   string until the first 0 byte. If maxBytesToRead is passed, and the string
   *   at [ptr, ptr+maxBytesToReadr[ contains a null byte in the middle, then the
   *   string will cut short at that byte index.
   * @param {boolean=} ignoreNul - If true, the function will not stop on a NUL character.
   * @return {string}
   */
  var UTF8ToString = (ptr, maxBytesToRead, ignoreNul) => {
      return ptr ? UTF8ArrayToString(HEAPU8, ptr, maxBytesToRead, ignoreNul) : '';
    };
  
  
  /** @type {!Int32Array} */
  var HEAP32;
  
  
  /** not-@type {!BigInt64Array} */
  var HEAP64;
  var SYSCALLS = {
  currentUmask:18,
  calculateAt(dirfd, path, allowEmpty) {
        if (PATH.isAbs(path)) {
          return path;
        }
        // relative path
        var dir;
        if (dirfd === -100) {
          dir = FS.cwd();
        } else {
          var dirstream = SYSCALLS.getStreamFromFD(dirfd);
          dir = dirstream.path;
        }
        if (path.length == 0) {
          if (!allowEmpty) {
            throw new FS.ErrnoError(44);;
          }
          return dir;
        }
        return dir + '/' + path;
      },
  writeStat(buf, stat) {
        HEAPU32[((buf)>>2)] = stat.dev;
        HEAPU32[(((buf)+(4))>>2)] = stat.mode;
        HEAPU32[(((buf)+(8))>>2)] = stat.nlink;
        HEAPU32[(((buf)+(12))>>2)] = stat.uid;
        HEAPU32[(((buf)+(16))>>2)] = stat.gid;
        HEAPU32[(((buf)+(20))>>2)] = stat.rdev;
        HEAP64[(((buf)+(24))>>3)] = BigInt(stat.size);
        HEAP32[(((buf)+(32))>>2)] = 4096;
        HEAP32[(((buf)+(36))>>2)] = stat.blocks;
        var atime = stat.atime.getTime();
        var mtime = stat.mtime.getTime();
        var ctime = stat.ctime.getTime();
        HEAP64[(((buf)+(40))>>3)] = BigInt(Math.floor(atime / 1000));
        HEAPU32[(((buf)+(48))>>2)] = (atime % 1000) * 1000 * 1000;
        HEAP64[(((buf)+(56))>>3)] = BigInt(Math.floor(mtime / 1000));
        HEAPU32[(((buf)+(64))>>2)] = (mtime % 1000) * 1000 * 1000;
        HEAP64[(((buf)+(72))>>3)] = BigInt(Math.floor(ctime / 1000));
        HEAPU32[(((buf)+(80))>>2)] = (ctime % 1000) * 1000 * 1000;
        HEAP64[(((buf)+(88))>>3)] = BigInt(stat.ino);
        return 0;
      },
  writeStatFs(buf, stats) {
        HEAPU32[(((buf)+(4))>>2)] = stats.bsize;
        HEAPU32[(((buf)+(60))>>2)] = stats.bsize;
        HEAP64[(((buf)+(8))>>3)] = BigInt(stats.blocks);
        HEAP64[(((buf)+(16))>>3)] = BigInt(stats.bfree);
        HEAP64[(((buf)+(24))>>3)] = BigInt(stats.bavail);
        HEAP64[(((buf)+(32))>>3)] = BigInt(stats.files);
        HEAP64[(((buf)+(40))>>3)] = BigInt(stats.ffree);
        HEAPU32[(((buf)+(48))>>2)] = stats.fsid;
        HEAPU32[(((buf)+(64))>>2)] = stats.flags;  // ST_NOSUID
        HEAPU32[(((buf)+(56))>>2)] = stats.namelen;
      },
  doMsync(addr, stream, len, flags, offset) {
        if (!FS.isFile(stream.node.mode)) {
          throw new FS.ErrnoError(43);
        }
        if (flags & 2) {
          // MAP_PRIVATE calls need not to be synced back to underlying fs
          return 0;
        }
        var buffer = HEAPU8.subarray(addr, addr + len);
        FS.msync(stream, buffer, offset, len, flags);
      },
  getStreamFromFD(fd) {
        var stream = FS.getStreamChecked(fd);
        return stream;
      },
  varargs:undefined,
  getStr(ptr) {
        var ret = UTF8ToString(ptr);
        return ret;
      },
  };
  function ___syscall_chdir(path) {
  try {
  
      path = SYSCALLS.getStr(path);
      FS.chdir(path);
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  function ___syscall_faccessat(dirfd, path, amode, flags) {
  try {
  
      path = SYSCALLS.getStr(path);
      path = SYSCALLS.calculateAt(dirfd, path);
      if (amode & ~7) {
        // need a valid mode
        return -28;
      }
      var lookup = FS.lookupPath(path, { follow: true });
      var node = lookup.node;
      if (!node) {
        return -44;
      }
      var perms = '';
      if (amode & 4) perms += 'r';
      if (amode & 2) perms += 'w';
      if (amode & 1) perms += 'x';
      if (perms /* otherwise, they've just passed F_OK */ && FS.nodePermissions(node, perms)) {
        return -2;
      }
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  var syscallGetVarargI = () => {
      // the `+` prepended here is necessary to convince the JSCompiler that varargs is indeed a number.
      var ret = HEAP32[((+SYSCALLS.varargs)>>2)];
      SYSCALLS.varargs += 4;
      return ret;
    };
  var syscallGetVarargP = syscallGetVarargI;
  
  
  
  /** @type {!Int16Array} */
  var HEAP16;
  function ___syscall_fcntl64(fd, cmd, varargs) {
  SYSCALLS.varargs = varargs;
  try {
  
      var stream = SYSCALLS.getStreamFromFD(fd);
      switch (cmd) {
        case 0: {
          var arg = syscallGetVarargI();
          if (arg < 0) {
            return -28;
          }
          while (FS.streams[arg]) {
            arg++;
          }
          var newStream;
          newStream = FS.dupStream(stream, arg);
          return newStream.fd;
        }
        case 1:
        case 2:
          return 0;  // FD_CLOEXEC makes no sense for a single process.
        case 3:
          return stream.flags;
        case 4: {
          var arg = syscallGetVarargI();
          var mask = 289792;
          stream.flags = (stream.flags & ~mask) | (arg & mask);
          return 0;
        }
        case 12: {
          var arg = syscallGetVarargP();
          var offset = 0;
          // We're always unlocked.
          HEAP16[(((arg)+(offset))>>1)] = 2;
          return 0;
        }
        case 13:
        case 14:
          // Pretend that the locking is successful. These are process-level locks,
          // and Emscripten programs are a single process. If we supported linking a
          // filesystem between programs, we'd need to do more here.
          // See https://github.com/emscripten-core/emscripten/issues/23697
          return 0;
      }
      return -28;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  function ___syscall_fstat64(fd, buf) {
  try {
  
      return SYSCALLS.writeStat(buf, FS.fstat(fd));
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  
  
  var stringToUTF8 = (str, outPtr, maxBytesToWrite) => {
      return stringToUTF8Array(str, HEAPU8, outPtr, maxBytesToWrite);
    };
  function ___syscall_getcwd(buf, size) {
  try {
  
      if (!size) return -28;
      var cwd = FS.cwd();
      var cwdLengthInBytes = lengthBytesUTF8(cwd) + 1;
      if (size < cwdLengthInBytes) return -68;
      stringToUTF8(cwd, buf, size);
      return cwdLengthInBytes;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  
  
  
  
  function ___syscall_getdents64(fd, dirp, count) {
  try {
  
      var stream = SYSCALLS.getStreamFromFD(fd)
      stream.getdents ||= FS.readdir(stream.path);
  
      var struct_size = 280;
      var pos = 0;
      var off = FS.llseek(stream, 0, 1);
  
      var startIdx = Math.floor(off / struct_size);
      var endIdx = Math.min(stream.getdents.length, startIdx + Math.floor(count/struct_size))
      for (var idx = startIdx; idx < endIdx; idx++) {
        var id;
        var type;
        var name = stream.getdents[idx];
        if (name === '.') {
          id = stream.node.id;
          type = 4;
        }
        else if (name === '..') {
          var lookup = FS.lookupPath(stream.path, { parent: true });
          id = lookup.node.id;
          type = 4;
        }
        else {
          var child;
          try {
            child = FS.lookupNode(stream.node, name);
          } catch (e) {
            // If the entry is not a directory, file, or symlink, nodefs
            // lookupNode will raise EINVAL. Skip these and continue.
            if (e?.errno === 28) {
              continue;
            }
            throw e;
          }
          id = child.id;
          type = FS.isChrdev(child.mode) ? 2 : // character device.
                 FS.isDir(child.mode) ? 4 :    // directory
                 FS.isLink(child.mode) ? 10 :   // symbolic link.
                 8;                            // regular file.
        }
        HEAP64[((dirp + pos)>>3)] = BigInt(id);
        HEAP64[(((dirp + pos)+(8))>>3)] = BigInt((idx + 1) * struct_size);
        HEAP16[(((dirp + pos)+(16))>>1)] = 280;
        HEAP8[(dirp + pos)+(18)] = type;
        stringToUTF8(name, dirp + pos + 19, 256);
        pos += struct_size;
      }
      FS.llseek(stream, idx * struct_size, 0);
      return pos;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  
  
  
  
  function ___syscall_ioctl(fd, op, varargs) {
  SYSCALLS.varargs = varargs;
  try {
  
      var stream = SYSCALLS.getStreamFromFD(fd);
      switch (op) {
        case 21509: {
          if (!stream.tty) return -59;
          return 0;
        }
        case 21505: {
          if (!stream.tty) return -59;
          if (stream.tty.ops.ioctl_tcgets) {
            var termios = stream.tty.ops.ioctl_tcgets(stream);
            var argp = syscallGetVarargP();
            HEAP32[((argp)>>2)] = termios.c_iflag || 0;
            HEAP32[(((argp)+(4))>>2)] = termios.c_oflag || 0;
            HEAP32[(((argp)+(8))>>2)] = termios.c_cflag || 0;
            HEAP32[(((argp)+(12))>>2)] = termios.c_lflag || 0;
            for (var i = 0; i < 32; i++) {
              HEAP8[(argp + i)+(17)] = termios.c_cc[i] || 0;
            }
            return 0;
          }
          return 0;
        }
        case 21510:
        case 21511:
        case 21512: {
          if (!stream.tty) return -59;
          return 0; // no-op, not actually adjusting terminal settings
        }
        case 21506:
        case 21507:
        case 21508: {
          if (!stream.tty) return -59;
          if (stream.tty.ops.ioctl_tcsets) {
            var argp = syscallGetVarargP();
            var c_iflag = HEAP32[((argp)>>2)];
            var c_oflag = HEAP32[(((argp)+(4))>>2)];
            var c_cflag = HEAP32[(((argp)+(8))>>2)];
            var c_lflag = HEAP32[(((argp)+(12))>>2)];
            var c_cc = []
            for (var i = 0; i < 32; i++) {
              c_cc.push(HEAP8[(argp + i)+(17)]);
            }
            return stream.tty.ops.ioctl_tcsets(stream.tty, op, { c_iflag, c_oflag, c_cflag, c_lflag, c_cc });
          }
          return 0; // no-op, not actually adjusting terminal settings
        }
        case 21519: {
          if (!stream.tty) return -59;
          var argp = syscallGetVarargP();
          HEAP32[((argp)>>2)] = 0;
          return 0;
        }
        case 21520: {
          if (!stream.tty) return -59;
          return -28; // not supported
        }
        case 21537:
        case 21531: {
          var argp = syscallGetVarargP();
          return FS.ioctl(stream, op, argp);
        }
        case 21523: {
          // TODO: in theory we should write to the winsize struct that gets
          // passed in, but for now musl doesn't read anything on it
          if (!stream.tty) return -59;
          if (stream.tty.ops.ioctl_tiocgwinsz) {
            var winsize = stream.tty.ops.ioctl_tiocgwinsz(stream.tty);
            var argp = syscallGetVarargP();
            HEAP16[((argp)>>1)] = winsize[0];
            HEAP16[(((argp)+(2))>>1)] = winsize[1];
          }
          return 0;
        }
        case 21524: {
          // TODO: technically, this ioctl call should change the window size.
          // but, since emscripten doesn't have any concept of a terminal window
          // yet, we'll just silently throw it away as we do TIOCGWINSZ
          if (!stream.tty) return -59;
          return 0;
        }
        case 21515: {
          if (!stream.tty) return -59;
          return 0;
        }
        default: return -28; // not supported
      }
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  function ___syscall_lstat64(path, buf) {
  try {
  
      path = SYSCALLS.getStr(path);
      return SYSCALLS.writeStat(buf, FS.lstat(path));
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  function ___syscall_mkdirat(dirfd, path, mode) {
  try {
  
      path = SYSCALLS.getStr(path);
      path = SYSCALLS.calculateAt(dirfd, path);
      mode &= ~SYSCALLS.currentUmask;
      FS.mkdir(path, mode, 0);
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  function ___syscall_newfstatat(dirfd, path, buf, flags) {
  try {
  
      path = SYSCALLS.getStr(path);
      var nofollow = flags & 256;
      var allowEmpty = flags & 4096;
      flags = flags & (~6400);
      path = SYSCALLS.calculateAt(dirfd, path, allowEmpty);
      return SYSCALLS.writeStat(buf, nofollow ? FS.lstat(path) : FS.stat(path));
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  
  function ___syscall_openat(dirfd, path, flags, varargs) {
  SYSCALLS.varargs = varargs;
  try {
  
      path = SYSCALLS.getStr(path);
      path = SYSCALLS.calculateAt(dirfd, path);
      var mode = varargs ? syscallGetVarargI() : 0;
      if (flags & 64) {
        mode &= ~SYSCALLS.currentUmask;
      }
      return FS.open(path, flags, mode).fd;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  var pollOne = (fd, events) => {
      var stream = FS.getStream(fd);
      if (!stream) return 32;
      // Streams without a poll handler (regular files, incl. NODERAWFS/NODEFS
      // which leave stream_ops unset) are treated as always readable+writable.
      var flags = stream.stream_ops?.poll?.(stream) ?? 5;
      return flags & (events | 8 | 16 | 32);
    };
  
  
  var doPollSync = (fds, nfds) => {
      var count = 0;
      for (var i = 0, pollfd = fds; i < nfds; i++, pollfd += 8) {
        var revents = pollOne(
          HEAP32[((pollfd)>>2)],
          HEAP16[(((pollfd)+(4))>>1)]);
        if (revents) count++;
        HEAP16[(((pollfd)+(6))>>1)] = revents;
      }
      return count;
    };
  
  
  
  
  var doPollAsync = (fds, nfds, timeout) => new Promise((resolve) => {
      var regs = [];
      var timer;
      var done = false;
      function derive() {
        var count = 0;
        for (var i = 0, pollfd = fds; i < nfds; i++, pollfd += 8) {
          var revents = pollOne(
            HEAP32[((pollfd)>>2)],
            HEAP16[(((pollfd)+(4))>>1)]);
          if (revents) count++;
          HEAP16[(((pollfd)+(6))>>1)] = revents;
        }
        return count;
      }
      function finish(count) {
        if (done) return;
        done = true;
        for (var r of regs) r.listeners.delete(r.entry);
        if (timer) clearTimeout(timer);
        resolve(count);
      }
      var count = derive();
      if (count || !timeout) {
        finish(count);
      } else {
        function recheck() {
          if (done) return;
          var c = derive();
          if (c) finish(c);
        }
        for (var i = 0, pollfd = fds; i < nfds; i++, pollfd += 8) {
          var stream = FS.getStream(HEAP32[((pollfd)>>2)]);
          if (stream) regs.push(stream.node.addListener(recheck));
        }
        if (timeout > 0) timer = setTimeout(() => finish(0), timeout);
      }
    });
  var ___syscall_poll = function(fds, nfds, timeout) {
    let innerFunc =  () => {
  
  try {
  
      const isAsyncContext = true;
      // When proxied from a worker (PTHREADS) or able to suspend (ASYNCIFY/JSPI),
      // block on the wait-queue. This must run for every timeout (including zero):
      // a proxied syscall's return is awaited by the caller thread, so it has to
      // be a Promise even for a probe.
      if (isAsyncContext) {
        return doPollAsync(fds, nfds, timeout);
      }
      var count = doPollSync(fds, nfds);
      return count;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  
  };
    return Asyncify.handleAsync(innerFunc);
  }
  ;
  ___syscall_poll.isAsync = true;

  
  
  
  function ___syscall_readlinkat(dirfd, path, buf, bufsize) {
  try {
  
      path = SYSCALLS.getStr(path);
      path = SYSCALLS.calculateAt(dirfd, path);
      if (bufsize <= 0) return -28;
      var ret = FS.readlink(path);
  
      var len = Math.min(bufsize, lengthBytesUTF8(ret));
      var endChar = HEAP8[buf+len];
      stringToUTF8(ret, buf, bufsize+1);
      // readlink is one of the rare functions that write out a C string, but does never append a null to the output buffer(!)
      // stringToUTF8() always appends a null byte, so restore the character under the null byte after the write.
      HEAP8[buf+len] = endChar;
      return len;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  function ___syscall_renameat(olddirfd, oldpath, newdirfd, newpath) {
  try {
  
      oldpath = SYSCALLS.getStr(oldpath);
      newpath = SYSCALLS.getStr(newpath);
      oldpath = SYSCALLS.calculateAt(olddirfd, oldpath);
      newpath = SYSCALLS.calculateAt(newdirfd, newpath);
      FS.rename(oldpath, newpath);
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  function ___syscall_stat64(path, buf) {
  try {
  
      path = SYSCALLS.getStr(path);
      return SYSCALLS.writeStat(buf, FS.stat(path));
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  function ___syscall_statfs64(path, size, buf) {
  try {
  
      SYSCALLS.writeStatFs(buf, FS.statfs(SYSCALLS.getStr(path)));
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  function ___syscall_unlinkat(dirfd, path, flags) {
  try {
  
      path = SYSCALLS.getStr(path);
      path = SYSCALLS.calculateAt(dirfd, path);
      if (!flags) {
        FS.unlink(path);
      } else if (flags === 512) {
        FS.rmdir(path);
      } else {
        return -28;
      }
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return -e.errno;
  }
  }
  

  var __abort_js = () =>
      abort('');

  var runtimeKeepaliveCounter = 0;
  var __emscripten_runtime_keepalive_clear = () => {
      noExitRuntime = false;
      runtimeKeepaliveCounter = 0;
    };

  var INT53_MAX = 9007199254740992;
  
  var INT53_MIN = -9007199254740992;
  var bigintToI53Checked = (num) => (num < INT53_MIN || num > INT53_MAX) ? NaN : Number(num);
  
  function __gmtime_js(time, tmPtr) {
    time = bigintToI53Checked(time);
  
  
      var date = new Date(time * 1000);
      if (isNaN(date.getTime())) {
        return 1;
      }
      HEAP32[((tmPtr)>>2)] = date.getUTCSeconds();
      HEAP32[(((tmPtr)+(4))>>2)] = date.getUTCMinutes();
      HEAP32[(((tmPtr)+(8))>>2)] = date.getUTCHours();
      HEAP32[(((tmPtr)+(12))>>2)] = date.getUTCDate();
      HEAP32[(((tmPtr)+(16))>>2)] = date.getUTCMonth();
      HEAP32[(((tmPtr)+(20))>>2)] = date.getUTCFullYear()-1900;
      HEAP32[(((tmPtr)+(24))>>2)] = date.getUTCDay();
      var start = Date.UTC(date.getUTCFullYear(), 0, 1, 0, 0, 0, 0);
      var yday = ((date.getTime() - start) / (1000 * 60 * 60 * 24))|0;
      HEAP32[(((tmPtr)+(28))>>2)] = yday;
      return 0;
    ;
  }

  var isLeapYear = (year) => year%4 === 0 && (year%100 !== 0 || year%400 === 0);
  
  var MONTH_DAYS_LEAP_CUMULATIVE = [0,31,60,91,121,152,182,213,244,274,305,335];
  
  var MONTH_DAYS_REGULAR_CUMULATIVE = [0,31,59,90,120,151,181,212,243,273,304,334];
  var ydayFromDate = (date) => {
      var leap = isLeapYear(date.getFullYear());
      var monthDaysCumulative = (leap ? MONTH_DAYS_LEAP_CUMULATIVE : MONTH_DAYS_REGULAR_CUMULATIVE);
      var yday = monthDaysCumulative[date.getMonth()] + date.getDate() - 1; // -1 since it's days since Jan 1
  
      return yday;
    };
  
  
  function __localtime_js(time, tmPtr) {
    time = bigintToI53Checked(time);
  
  
      var date = new Date(time*1000);
      if (isNaN(date.getTime())) {
        return 1;
      }
      HEAP32[((tmPtr)>>2)] = date.getSeconds();
      HEAP32[(((tmPtr)+(4))>>2)] = date.getMinutes();
      HEAP32[(((tmPtr)+(8))>>2)] = date.getHours();
      HEAP32[(((tmPtr)+(12))>>2)] = date.getDate();
      HEAP32[(((tmPtr)+(16))>>2)] = date.getMonth();
      HEAP32[(((tmPtr)+(20))>>2)] = date.getFullYear()-1900;
      HEAP32[(((tmPtr)+(24))>>2)] = date.getDay();
  
      var yday = ydayFromDate(date)|0;
      HEAP32[(((tmPtr)+(28))>>2)] = yday;
      HEAP32[(((tmPtr)+(36))>>2)] = -(date.getTimezoneOffset() * 60);
  
      // Attention: DST is in December in South, and some regions don't have DST at all.
      var start = new Date(date.getFullYear(), 0, 1);
      var summerOffset = new Date(date.getFullYear(), 6, 1).getTimezoneOffset();
      var winterOffset = start.getTimezoneOffset();
      var dst = (summerOffset != winterOffset && date.getTimezoneOffset() == Math.min(winterOffset, summerOffset))|0;
      HEAP32[(((tmPtr)+(32))>>2)] = dst;
      return 0;
    ;
  }

  
  
  var __tzset_js = (timezone, daylight, std_name, dst_name) => {
      // TODO: Use (malleable) environment variables instead of system settings.
      var currentYear = new Date().getFullYear();
      var winter = new Date(currentYear, 0, 1);
      var summer = new Date(currentYear, 6, 1);
      var winterOffset = winter.getTimezoneOffset();
      var summerOffset = summer.getTimezoneOffset();
  
      // Local standard timezone offset. Local standard time is not adjusted for
      // daylight savings.  This code uses the fact that getTimezoneOffset returns
      // a greater value during Standard Time versus Daylight Saving Time (DST).
      // Thus it determines the expected output during Standard Time, and it
      // compares whether the output of the given date the same (Standard) or less
      // (DST).
      var stdTimezoneOffset = Math.max(winterOffset, summerOffset);
  
      // timezone is specified as seconds west of UTC ("The external variable
      // `timezone` shall be set to the difference, in seconds, between
      // Coordinated Universal Time (UTC) and local standard time."), the same
      // as returned by stdTimezoneOffset.
      // See http://pubs.opengroup.org/onlinepubs/009695399/functions/tzset.html
      HEAPU32[((timezone)>>2)] = stdTimezoneOffset * 60;
  
      HEAP32[((daylight)>>2)] = Number(winterOffset != summerOffset);
  
      var extractZone = (timezoneOffset) => {
        // Why inverse sign?
        // Read here https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getTimezoneOffset
        var sign = timezoneOffset >= 0 ? '-' : '+';
  
        var absOffset = Math.abs(timezoneOffset)
        var hours = String(Math.floor(absOffset / 60)).padStart(2, '0');
        var minutes = String(absOffset % 60).padStart(2, '0');
  
        return `UTC${sign}${hours}${minutes}`;
      }
  
      var winterName = extractZone(winterOffset);
      var summerName = extractZone(summerOffset);
      if (summerOffset < winterOffset) {
        // Northern hemisphere
        stringToUTF8(winterName, std_name, 17);
        stringToUTF8(summerName, dst_name, 17);
      } else {
        stringToUTF8(winterName, dst_name, 17);
        stringToUTF8(summerName, std_name, 17);
      }
    };

  
  var _emscripten_set_main_loop_timing = (mode, value) => {
      MainLoop.timingMode = mode;
      MainLoop.timingValue = value;
  
      if (!MainLoop.func) {
        return 1; // Return non-zero on failure, can't set timing mode when there is no main loop.
      }
  
      if (mode == 0) {
        MainLoop.scheduler = function MainLoop_scheduler_setTimeout() {
          var timeUntilNextTick = Math.max(0, MainLoop.tickStartTime + value - _emscripten_get_now())|0;
          setTimeout(MainLoop.runner, timeUntilNextTick); // doing this each time means that on exception, we stop
        };
      } else if (mode == 1) {
        MainLoop.scheduler = function MainLoop_scheduler_rAF() {
          MainLoop.requestAnimationFrame(MainLoop.runner);
        };
      } else {
        if (!MainLoop.setImmediate) {
          if (globalThis.scheduler) {
            // Some modern browsers implement scheduler.postTask, but not all.
            MainLoop.setImmediate = scheduler.postTask.bind(scheduler);
          } else if (globalThis.setImmediate) {
            MainLoop.setImmediate = setImmediate;
          } else {
            // Emulate setImmediate. (note: not a complete polyfill, we don't emulate clearImmediate() to keep code size to minimum, since not needed)
            var setImmediates = [];
            var emscriptenMainLoopMessageId = 'setimmediate';
            /** @param {Event} event */
            var MainLoop_setImmediate_messageHandler = (event) => {
              if (event.data === emscriptenMainLoopMessageId) {
                event.stopPropagation();
                setImmediates.shift()();
              }
            };
            addEventListener('message', MainLoop_setImmediate_messageHandler, true);
            MainLoop.setImmediate = /** @type{function(function(): ?, ...?): number} */((func) => {
              setImmediates.push(func);
              if (ENVIRONMENT_IS_WORKER) {
                // The postMessge API in a Worker, sends message to the main
                // thread and does not support the `targetOrigin` (*) argument.
                postMessage(emscriptenMainLoopMessageId);
              } else {
                postMessage(emscriptenMainLoopMessageId, '*');
              }
            });
          }
        }
        MainLoop.scheduler = function MainLoop_scheduler_setImmediate() {
          MainLoop.setImmediate(MainLoop.runner);
        };
      }
      return 0;
    };
  
  var _emscripten_get_now = () => performance.now();
  
  
  var keepRuntimeAlive = () => noExitRuntime || runtimeKeepaliveCounter > 0;
  var _proc_exit = (code) => {
      EXITSTATUS = code;
      if (!keepRuntimeAlive()) {
        Module['onExit']?.(code);
        ABORT = true;
      }
      quit_(code, new ExitStatus(code));
    };
  /** @param {boolean|number=} implicit */
  var exitJS = (status, implicit) => {
      EXITSTATUS = status;
  
      _proc_exit(status);
    };
  var _exit = exitJS;
  
  var handleException = (e) => {
      // Certain exception types we do not treat as errors since they are used for
      // internal control flow.
      // 1. ExitStatus, which is thrown by exit()
      // 2. "unwind", which is thrown by emscripten_unwind_to_js_event_loop() and others
      //    that wish to return to JS event loop.
      if (e instanceof ExitStatus || e == 'unwind') {
        return EXITSTATUS;
      }
      quit_(1, e);
    };
  
  var maybeExit = () => {
      if (!keepRuntimeAlive()) {
        try {
          _exit(EXITSTATUS);
        } catch (e) {
          handleException(e);
        }
      }
    };
  
    /**
   * @param {number=} arg
   * @param {boolean=} noSetTiming
   */
  var setMainLoop = (iterFunc, fps, simulateInfiniteLoop, arg, noSetTiming) => {
      MainLoop.func = iterFunc;
      MainLoop.arg = arg;
  
      var thisMainLoopId = MainLoop.currentlyRunningMainloop;
      function checkIsRunning() {
        if (thisMainLoopId < MainLoop.currentlyRunningMainloop) {
          maybeExit();
          return false;
        }
        return true;
      }
  
      // We create the loop runner here but it is not actually running until
      // _emscripten_set_main_loop_timing is called (which might happen at a
      // later time).
      MainLoop.runner = function MainLoop_runner() {
        if (ABORT) return;
        if (MainLoop.queue.length > 0) {
          var start = Date.now();
          var blocker = MainLoop.queue.shift();
          blocker.func(blocker.arg);
          if (MainLoop.remainingBlockers) {
            var remaining = MainLoop.remainingBlockers;
            var next = remaining%1 == 0 ? remaining-1 : Math.floor(remaining);
            if (blocker.counted) {
              MainLoop.remainingBlockers = next;
            } else {
              // not counted, but move the progress along a tiny bit
              next = next + 0.5; // do not steal all the next one's progress
              MainLoop.remainingBlockers = (8*remaining + next)/9;
            }
          }
          MainLoop.updateStatus();
  
          // catches pause/resume main loop from blocker execution
          if (!checkIsRunning()) return;
  
          setTimeout(MainLoop.runner, 0);
          return;
        }
  
        // catch pauses from non-main loop sources
        if (!checkIsRunning()) return;
  
        // Implement very basic swap interval control
        MainLoop.currentFrameNumber = MainLoop.currentFrameNumber + 1 | 0;
        if (MainLoop.timingMode == 1 && MainLoop.timingValue > 1 && MainLoop.currentFrameNumber % MainLoop.timingValue != 0) {
          // Not the scheduled time to render this frame - skip.
          MainLoop.scheduler();
          return;
        } else if (MainLoop.timingMode == 0) {
          MainLoop.tickStartTime = _emscripten_get_now();
        }
  
        MainLoop.runIter(iterFunc);
  
        // catch pauses from the main loop itself
        if (!checkIsRunning()) return;
  
        MainLoop.scheduler();
      }
  
      if (!noSetTiming) {
        if (fps > 0) {
          _emscripten_set_main_loop_timing(0, 1000.0 / fps);
        } else {
          // Do rAF by rendering each frame (no decimating)
          _emscripten_set_main_loop_timing(1, 1);
        }
  
        MainLoop.scheduler();
      }
  
      if (simulateInfiniteLoop) {
        throw 'unwind';
      }
    };
  
  
  var callUserCallback = (func) => {
      if (ABORT) {
        return;
      }
      try {
        return func();
      } catch (e) {
        handleException(e);
      } finally {
        maybeExit();
      }
    };
  
  var MainLoop = {
  func:null,
  scheduler:null,
  currentlyRunningMainloop:0,
  arg:0,
  timingMode:0,
  timingValue:0,
  currentFrameNumber:0,
  queue:[],
  preMainLoop:[],
  postMainLoop:[],
  pause() {
        if (MainLoop.scheduler) {
          MainLoop.scheduler = null;
          // Incrementing this signals the previous main loop that it's now become old, and it must return.
          MainLoop.currentlyRunningMainloop++;
          
        }
      },
  resume() {
        MainLoop.currentlyRunningMainloop++;
        var timingMode = MainLoop.timingMode;
        var timingValue = MainLoop.timingValue;
        var func = MainLoop.func;
        MainLoop.func = null;
        // do not set timing and call scheduler, we will do it on the next lines
        setMainLoop(func, 0, false, MainLoop.arg, true);
        _emscripten_set_main_loop_timing(timingMode, timingValue);
        MainLoop.scheduler();
      },
  updateStatus() {
        if (Module['setStatus']) {
          var message = Module['statusMessage'] || 'Please wait...';
          var remaining = MainLoop.remainingBlockers ?? 0;
          var expected = MainLoop.expectedBlockers ?? 0;
          if (remaining) {
            if (remaining < expected) {
              Module['setStatus'](`{message} ({expected - remaining}/{expected})`);
            } else {
              Module['setStatus'](message);
            }
          } else {
            Module['setStatus']('');
          }
        }
      },
  init() {
      },
  runIter(func) {
        if (ABORT) return;
        for (var pre of MainLoop.preMainLoop) {
          if (pre() === false) {
            return; // |return false| skips a frame
          }
        }
        callUserCallback(func);
        for (var post of MainLoop.postMainLoop) {
          post();
        }
      },
  nextRAF:0,
  fakeRequestAnimationFrame(func) {
        // try to keep 60fps between calls to here
        var now = Date.now();
        if (!MainLoop.nextRAF) {
          MainLoop.nextRAF = now + 1000/60;
        } else {
          while (now + 2 >= MainLoop.nextRAF) { // fudge a little, to avoid timer jitter causing us to do lots of delay:0
            MainLoop.nextRAF += 1000/60;
          }
        }
        var delay = Math.max(MainLoop.nextRAF - now, 0);
        setTimeout(func, delay);
      },
  requestAnimationFrame(func) {
        if (globalThis.requestAnimationFrame) {
          requestAnimationFrame(func);
        } else {
          MainLoop.fakeRequestAnimationFrame(func);
        }
      },
  };
  
  var AL = {
  QUEUE_INTERVAL:25,
  QUEUE_LOOKAHEAD:0.1,
  DEVICE_NAME:"Emscripten OpenAL",
  CAPTURE_DEVICE_NAME:"Emscripten OpenAL capture",
  ALC_EXTENSIONS:{
  ALC_EXT_capture:true,
  ALC_SOFT_pause_device:true,
  ALC_SOFT_HRTF:true,
  },
  AL_EXTENSIONS:{
  AL_EXT_float32:true,
  AL_SOFT_loop_points:true,
  AL_SOFT_source_length:true,
  AL_EXT_source_distance_model:true,
  AL_SOFT_source_spatialize:true,
  },
  _alcErr:0,
  alcErr:0,
  deviceRefCounts:{
  },
  alcStringCache:{
  },
  paused:false,
  stringCache:{
  },
  contexts:{
  },
  currentCtx:null,
  buffers:{
  0:{
  id:0,
  refCount:0,
  audioBuf:null,
  frequency:0,
  bytesPerSample:2,
  channels:1,
  length:0,
  },
  },
  paramArray:[],
  _nextId:1,
  newId:() => AL.freeIds.length > 0 ? AL.freeIds.pop() : AL._nextId++,
  freeIds:[],
  scheduleContextAudio:(ctx) => {
        // If we are animating using the requestAnimationFrame method, then the main loop does not run when in the background.
        // To give a perfect glitch-free audio stop when switching from foreground to background, we need to avoid updating
        // audio altogether when in the background, so detect that case and kill audio buffer streaming if so.
        if (MainLoop.timingMode === 1 && document['visibilityState'] != 'visible') {
          return;
        }
  
        for (var i in ctx.sources) {
          AL.scheduleSourceAudio(ctx.sources[i]);
        }
      },
  scheduleSourceAudio:(src, lookahead) => {
        // See comment on scheduleContextAudio above.
        if (MainLoop.timingMode === 1 && document['visibilityState'] != 'visible') {
          return;
        }
        if (src.state !== 4114) {
          return;
        }
  
        var currentTime = AL.updateSourceTime(src);
  
        var startTime = src.bufStartTime;
        var startOffset = src.bufOffset;
        var bufCursor = src.bufsProcessed;
  
        // Advance past any audio that is already scheduled
        for (var i = 0; i < src.audioQueue.length; i++) {
          var audioSrc = src.audioQueue[i];
          startTime = audioSrc._startTime + audioSrc._duration;
          startOffset = 0.0;
          bufCursor += audioSrc._skipCount + 1;
        }
  
        if (!lookahead) {
          lookahead = AL.QUEUE_LOOKAHEAD;
        }
        var lookaheadTime = currentTime + lookahead;
        var skipCount = 0;
        while (startTime < lookaheadTime) {
          if (bufCursor >= src.bufQueue.length) {
            if (src.looping) {
              bufCursor %= src.bufQueue.length;
            } else {
              break;
            }
          }
  
          var buf = src.bufQueue[bufCursor % src.bufQueue.length];
          // If the buffer contains no data, skip it
          if (!buf.length) {
            skipCount++;
            // If we've gone through the whole queue and everything is 0 length, just give up
            if (skipCount === src.bufQueue.length) {
              break;
            }
          } else {
            var audioSrc = src.context.audioCtx.createBufferSource();
            audioSrc.buffer = buf.audioBuf;
            audioSrc.playbackRate.value = src.playbackRate;
            if (buf.audioBuf._loopStart || buf.audioBuf._loopEnd) {
              audioSrc.loopStart = buf.audioBuf._loopStart;
              audioSrc.loopEnd = buf.audioBuf._loopEnd;
            }
  
            var duration = 0.0;
            // If the source is a looping static buffer, use native looping for gapless playback
            if (src.type === 4136 && src.looping) {
              duration = Number.POSITIVE_INFINITY;
              audioSrc.loop = true;
              if (buf.audioBuf._loopStart) {
                audioSrc.loopStart = buf.audioBuf._loopStart;
              }
              if (buf.audioBuf._loopEnd) {
                audioSrc.loopEnd = buf.audioBuf._loopEnd;
              }
            } else {
              duration = (buf.audioBuf.duration - startOffset) / src.playbackRate;
            }
  
            audioSrc._startOffset = startOffset;
            audioSrc._duration = duration;
            audioSrc._skipCount = skipCount;
            skipCount = 0;
  
            audioSrc.connect(src.gain);
  
            if (typeof audioSrc.start != 'undefined') {
              // Sample the current time as late as possible to mitigate drift
              startTime = Math.max(startTime, src.context.audioCtx.currentTime);
              audioSrc.start(startTime, startOffset);
            } else if (typeof audioSrc.noteOn != 'undefined') {
              startTime = Math.max(startTime, src.context.audioCtx.currentTime);
              audioSrc.noteOn(startTime);
            }
            audioSrc._startTime = startTime;
            src.audioQueue.push(audioSrc);
  
            startTime += duration;
          }
  
          startOffset = 0.0;
          bufCursor++;
        }
      },
  updateSourceTime:(src) => {
        var currentTime = src.context.audioCtx.currentTime;
        if (src.state !== 4114) {
          return currentTime;
        }
  
        // if the start time is unset, determine it based on the current offset.
        // This will be the case when a source is resumed after being paused, and
        // allows us to pretend that the source actually started playing some time
        // in the past such that it would just now have reached the stored offset.
        if (!isFinite(src.bufStartTime)) {
          src.bufStartTime = currentTime - src.bufOffset / src.playbackRate;
          src.bufOffset = 0.0;
        }
  
        var nextStartTime = 0.0;
        while (src.audioQueue.length) {
          var audioSrc = src.audioQueue[0];
          src.bufsProcessed += audioSrc._skipCount;
          nextStartTime = audioSrc._startTime + audioSrc._duration; // n.b. audioSrc._duration already factors in playbackRate, so no divide by src.playbackRate on it.
  
          if (currentTime < nextStartTime) {
            break;
          }
  
          src.audioQueue.shift();
          src.bufStartTime = nextStartTime;
          src.bufOffset = 0.0;
          src.bufsProcessed++;
        }
  
        if (src.bufsProcessed >= src.bufQueue.length && !src.looping) {
          // The source has played its entire queue and is non-looping, so just mark it as stopped.
          AL.setSourceState(src, 4116);
        } else if (src.type === 4136 && src.looping) {
          // If the source is a looping static buffer, determine the buffer offset based on the loop points
          var buf = src.bufQueue[0];
          if (!buf.length) {
            src.bufOffset = 0.0;
          } else {
            var delta = (currentTime - src.bufStartTime) * src.playbackRate;
            var loopStart = buf.audioBuf._loopStart ?? 0.0;
            var loopEnd = buf.audioBuf._loopEnd ?? buf.audioBuf.duration;
            if (loopEnd <= loopStart) {
              loopEnd = buf.audioBuf.duration;
            }
  
            if (delta < loopEnd) {
              src.bufOffset = delta;
            } else {
              src.bufOffset = loopStart + (delta - loopStart) % (loopEnd - loopStart);
            }
          }
        } else if (src.audioQueue[0]) {
          // The source is still actively playing, so we just need to calculate where we are in the current buffer
          // so it can be remembered if the source gets paused.
          src.bufOffset = (currentTime - src.audioQueue[0]._startTime) * src.playbackRate;
        } else {
          // The source hasn't finished yet, but there is no scheduled audio left for it. This can be because
          // the source has just been started/resumed, or due to an underrun caused by a long blocking operation.
          // We need to determine what state we would be in by this point in time so that when we next schedule
          // audio playback, it will be just as if no underrun occurred.
  
          if (src.type !== 4136 && src.looping) {
            // if the source is a looping buffer queue, let's first calculate the queue duration, so we can
            // quickly fast forward past any full loops of the queue and only worry about the remainder.
            var srcDuration = AL.sourceDuration(src) / src.playbackRate;
            if (srcDuration > 0.0) {
              src.bufStartTime += Math.floor((currentTime - src.bufStartTime) / srcDuration) * srcDuration;
            }
          }
  
          // Since we've already skipped any full-queue loops if there were any, we just need to find
          // out where in the queue the remaining time puts us, which won't require stepping through the
          // entire queue more than once.
          for (var i = 0; i < src.bufQueue.length; i++) {
            if (src.bufsProcessed >= src.bufQueue.length) {
              if (src.looping) {
                src.bufsProcessed %= src.bufQueue.length;
              } else {
                AL.setSourceState(src, 4116);
                break;
              }
            }
  
            var buf = src.bufQueue[src.bufsProcessed];
            if (buf.length > 0) {
              nextStartTime = src.bufStartTime + buf.audioBuf.duration / src.playbackRate;
  
              if (currentTime < nextStartTime) {
                src.bufOffset = (currentTime - src.bufStartTime) * src.playbackRate;
                break;
              }
  
              src.bufStartTime = nextStartTime;
            }
  
            src.bufOffset = 0.0;
            src.bufsProcessed++;
          }
        }
  
        return currentTime;
      },
  cancelPendingSourceAudio:(src) => {
        AL.updateSourceTime(src);
  
        for (var i = 1; i < src.audioQueue.length; i++) {
          var audioSrc = src.audioQueue[i];
          audioSrc.stop();
        }
  
        if (src.audioQueue.length > 1) {
          src.audioQueue.length = 1;
        }
      },
  stopSourceAudio:(src) => {
        for (var audioSrc of src.audioQueue) {
          audioSrc.stop();
        }
        src.audioQueue.length = 0;
      },
  setSourceState:(src, state) => {
        if (state === 4114) {
          if (src.state === 4114 || src.state == 4116) {
            src.bufsProcessed = 0;
            src.bufOffset = 0.0;
          } else {
          }
  
          AL.stopSourceAudio(src);
  
          src.state = 4114;
          src.bufStartTime = Number.NEGATIVE_INFINITY;
          AL.scheduleSourceAudio(src);
        } else if (state === 4115) {
          if (src.state === 4114) {
            // Store off the current offset to restore with on resume.
            AL.updateSourceTime(src);
            AL.stopSourceAudio(src);
  
            src.state = 4115;
          }
        } else if (state === 4116) {
          if (src.state !== 4113) {
            src.state = 4116;
            src.bufsProcessed = src.bufQueue.length;
            src.bufStartTime = Number.NEGATIVE_INFINITY;
            src.bufOffset = 0.0;
            AL.stopSourceAudio(src);
          }
        } else if (state === 4113) {
          if (src.state !== 4113) {
            src.state = 4113;
            src.bufsProcessed = 0;
            src.bufStartTime = Number.NEGATIVE_INFINITY;
            src.bufOffset = 0.0;
            AL.stopSourceAudio(src);
          }
        }
      },
  initSourcePanner:(src) => {
        if (src.type === 0x1030 /* AL_UNDETERMINED */) {
          return;
        }
  
        // Find the first non-zero buffer in the queue to determine the proper format
        var templateBuf = AL.buffers[0];
        for (var buf of src.bufQueue) {
          if (buf.id) {
            templateBuf = buf;
            break;
          }
        }
        // Create a panner if AL_SOURCE_SPATIALIZE_SOFT is set to true, or alternatively if it's set to auto and the source is mono
        if (src.spatialize === 1 || (src.spatialize === 2 /* AL_AUTO_SOFT */ && templateBuf.channels === 1)) {
          if (src.panner) {
            return;
          }
          src.panner = src.context.audioCtx.createPanner();
  
          AL.updateSourceGlobal(src);
          AL.updateSourceSpace(src);
  
          src.panner.connect(src.context.gain);
          src.gain.disconnect();
          src.gain.connect(src.panner);
        } else {
          if (!src.panner) {
            return;
          }
  
          src.panner.disconnect();
          src.gain.disconnect();
          src.gain.connect(src.context.gain);
          src.panner = null;
        }
      },
  updateContextGlobal:(ctx) => {
        for (var i in ctx.sources) {
          AL.updateSourceGlobal(ctx.sources[i]);
        }
      },
  updateSourceGlobal:(src) => {
        var panner = src.panner;
        if (!panner) {
          return;
        }
  
        panner.refDistance = src.refDistance;
        panner.maxDistance = src.maxDistance;
        panner.rolloffFactor = src.rolloffFactor;
  
        panner.panningModel = src.context.hrtf ? 'HRTF' : 'equalpower';
  
        // Use the source's distance model if AL_SOURCE_DISTANCE_MODEL is enabled
        var distanceModel = src.context.sourceDistanceModel ? src.distanceModel : src.context.distanceModel;
        switch (distanceModel) {
        case 0:
          panner.distanceModel = 'inverse';
          panner.refDistance = 3.40282e38 /* FLT_MAX */;
          break;
        case 0xd001 /* AL_INVERSE_DISTANCE */:
        case 0xd002 /* AL_INVERSE_DISTANCE_CLAMPED */:
          panner.distanceModel = 'inverse';
          break;
        case 0xd003 /* AL_LINEAR_DISTANCE */:
        case 0xd004 /* AL_LINEAR_DISTANCE_CLAMPED */:
          panner.distanceModel = 'linear';
          break;
        case 0xd005 /* AL_EXPONENT_DISTANCE */:
        case 0xd006 /* AL_EXPONENT_DISTANCE_CLAMPED */:
          panner.distanceModel = 'exponential';
          break;
        }
      },
  updateListenerSpace:(ctx) => {
        var listener = ctx.audioCtx.listener;
        if (listener.positionX) {
          listener.positionX.value = ctx.listener.position[0];
          listener.positionY.value = ctx.listener.position[1];
          listener.positionZ.value = ctx.listener.position[2];
        } else {
          listener.setPosition(ctx.listener.position[0], ctx.listener.position[1], ctx.listener.position[2]);
        }
        if (listener.forwardX) {
          listener.forwardX.value = ctx.listener.direction[0];
          listener.forwardY.value = ctx.listener.direction[1];
          listener.forwardZ.value = ctx.listener.direction[2];
          listener.upX.value = ctx.listener.up[0];
          listener.upY.value = ctx.listener.up[1];
          listener.upZ.value = ctx.listener.up[2];
        } else {
          listener.setOrientation(
            ctx.listener.direction[0], ctx.listener.direction[1], ctx.listener.direction[2],
            ctx.listener.up[0], ctx.listener.up[1], ctx.listener.up[2]);
        }
  
        // Update sources that are relative to the listener
        for (var i in ctx.sources) {
          AL.updateSourceSpace(ctx.sources[i]);
        }
      },
  updateSourceSpace:(src) => {
        if (!src.panner) {
          return;
        }
        var panner = src.panner;
  
        var posX = src.position[0];
        var posY = src.position[1];
        var posZ = src.position[2];
        var dirX = src.direction[0];
        var dirY = src.direction[1];
        var dirZ = src.direction[2];
  
        var listener = src.context.listener;
        var lPosX = listener.position[0];
        var lPosY = listener.position[1];
        var lPosZ = listener.position[2];
  
        // WebAudio does spatialization in world-space coordinates, meaning both the buffer sources and
        // the listener position are in the same absolute coordinate system relative to a fixed origin.
        // By default, OpenAL works this way as well, but it also provides a "listener relative" mode, where
        // a buffer source's coordinates are interpreted not in absolute world space, but as being relative
        // to the listener object itself, so as the listener moves the source appears to move with it
        // with no update required. Since web audio does not support this mode, we must transform the source
        // coordinates from listener-relative space to absolute world space.
        //
        // We do this via affine transformation matrices applied to the source position and source direction.
        // A change-of-basis converts from listener-space displacements to world-space displacements,
        // which must be done for both the source position and direction. Lastly, the source position must be
        // added to the listener position to get the final source position, since the source position represents
        // a displacement from the listener.
        if (src.relative) {
          // Negate the listener direction since forward is -Z.
          var lBackX = -listener.direction[0];
          var lBackY = -listener.direction[1];
          var lBackZ = -listener.direction[2];
          var lUpX = listener.up[0];
          var lUpY = listener.up[1];
          var lUpZ = listener.up[2];
  
          var inverseMagnitude = (x, y, z) => {
            var length = Math.sqrt(x * x + y * y + z * z);
  
            if (length < Number.EPSILON) {
              return 0.0;
            }
  
            return 1.0 / length;
          };
  
          // Normalize the Back vector
          var invMag = inverseMagnitude(lBackX, lBackY, lBackZ);
          lBackX *= invMag;
          lBackY *= invMag;
          lBackZ *= invMag;
  
          // ...and the Up vector
          invMag = inverseMagnitude(lUpX, lUpY, lUpZ);
          lUpX *= invMag;
          lUpY *= invMag;
          lUpZ *= invMag;
  
          // Calculate the Right vector as the cross product of the Up and Back vectors
          var lRightX = (lUpY * lBackZ - lUpZ * lBackY);
          var lRightY = (lUpZ * lBackX - lUpX * lBackZ);
          var lRightZ = (lUpX * lBackY - lUpY * lBackX);
  
          // Back and Up might not be exactly perpendicular, so the cross product also needs normalization
          invMag = inverseMagnitude(lRightX, lRightY, lRightZ);
          lRightX *= invMag;
          lRightY *= invMag;
          lRightZ *= invMag;
  
          // Recompute Up from the now orthonormal Right and Back vectors so we have a fully orthonormal basis
          lUpX = (lBackY * lRightZ - lBackZ * lRightY);
          lUpY = (lBackZ * lRightX - lBackX * lRightZ);
          lUpZ = (lBackX * lRightY - lBackY * lRightX);
  
          var oldX = dirX;
          var oldY = dirY;
          var oldZ = dirZ;
  
          // Use our 3 vectors to apply a change-of-basis matrix to the source direction
          dirX = oldX * lRightX + oldY * lUpX + oldZ * lBackX;
          dirY = oldX * lRightY + oldY * lUpY + oldZ * lBackY;
          dirZ = oldX * lRightZ + oldY * lUpZ + oldZ * lBackZ;
  
          oldX = posX;
          oldY = posY;
          oldZ = posZ;
  
          // ...and to the source position
          posX = oldX * lRightX + oldY * lUpX + oldZ * lBackX;
          posY = oldX * lRightY + oldY * lUpY + oldZ * lBackY;
          posZ = oldX * lRightZ + oldY * lUpZ + oldZ * lBackZ;
  
          // The change-of-basis corrects the orientation, but the origin is still the listener.
          // Translate the source position by the listener position to finish.
          posX += lPosX;
          posY += lPosY;
          posZ += lPosZ;
        }
  
        if (panner.positionX) {
          // Assigning to panner.positionX/Y/Z unnecessarily seems to cause performance issues
          // See https://github.com/emscripten-core/emscripten/issues/15847
  
          if (posX != panner.positionX.value) panner.positionX.value = posX;
          if (posY != panner.positionY.value) panner.positionY.value = posY;
          if (posZ != panner.positionZ.value) panner.positionZ.value = posZ;
        } else {
          panner.setPosition(posX, posY, posZ);
        }
        if (panner.orientationX) {
          // Assigning to panner.orientation/Y/Z unnecessarily seems to cause performance issues
          // See https://github.com/emscripten-core/emscripten/issues/15847
  
          if (dirX != panner.orientationX.value) panner.orientationX.value = dirX;
          if (dirY != panner.orientationY.value) panner.orientationY.value = dirY;
          if (dirZ != panner.orientationZ.value) panner.orientationZ.value = dirZ;
        } else {
          panner.setOrientation(dirX, dirY, dirZ);
        }
  
        var oldShift = src.dopplerShift;
        var velX = src.velocity[0];
        var velY = src.velocity[1];
        var velZ = src.velocity[2];
        var lVelX = listener.velocity[0];
        var lVelY = listener.velocity[1];
        var lVelZ = listener.velocity[2];
        if (posX === lPosX && posY === lPosY && posZ === lPosZ
          || velX === lVelX && velY === lVelY && velZ === lVelZ)
        {
          src.dopplerShift = 1.0;
        } else {
          // Doppler algorithm from 1.1 spec
          var speedOfSound = src.context.speedOfSound;
          var dopplerFactor = src.context.dopplerFactor;
  
          var slX = lPosX - posX;
          var slY = lPosY - posY;
          var slZ = lPosZ - posZ;
  
          var magSl = Math.sqrt(slX * slX + slY * slY + slZ * slZ);
          var vls = (slX * lVelX + slY * lVelY + slZ * lVelZ) / magSl;
          var vss = (slX * velX + slY * velY + slZ * velZ) / magSl;
  
          vls = Math.min(vls, speedOfSound / dopplerFactor);
          vss = Math.min(vss, speedOfSound / dopplerFactor);
  
          src.dopplerShift = (speedOfSound - dopplerFactor * vls) / (speedOfSound - dopplerFactor * vss);
        }
        if (src.dopplerShift !== oldShift) {
          AL.updateSourceRate(src);
        }
      },
  updateSourceRate:(src) => {
        if (src.state === 4114) {
          // clear scheduled buffers
          AL.cancelPendingSourceAudio(src);
  
          var audioSrc = src.audioQueue[0];
          if (!audioSrc) {
            return; // It is possible that AL.scheduleContextAudio() has not yet fed the next buffer, if so, skip.
          }
  
          var duration;
          if (src.type === 4136 && src.looping) {
            duration = Number.POSITIVE_INFINITY;
          } else {
            // audioSrc._duration is expressed after factoring in playbackRate, so when changing playback rate, need
            // to recompute/rescale the rate to the new playback speed.
            duration = (audioSrc.buffer.duration - audioSrc._startOffset) / src.playbackRate;
          }
  
          audioSrc._duration = duration;
          audioSrc.playbackRate.value = src.playbackRate;
  
          // reschedule buffers with the new playbackRate
          AL.scheduleSourceAudio(src);
        }
      },
  sourceDuration:(src) => {
        var length = 0.0;
        for (var buf of src.bufQueue) {
          length += buf.audioBuf?.duration ?? 0.0;
        }
        return length;
      },
  sourceTell:(src) => {
        AL.updateSourceTime(src);
  
        var offset = 0.0;
        for (var i = 0; i < src.bufsProcessed; i++) {
          offset += src.bufQueue[i].audioBuf?.duration ?? 0.0;
        }
        offset += src.bufOffset;
  
        return offset;
      },
  sourceSeek:(src, offset) => {
        var playing = src.state == 4114;
        if (playing) {
          AL.setSourceState(src, 4113);
        }
  
        src.bufsProcessed = 0;
        for (var buf of src.bufQueue) {
          var duration = buf.audioBuf?.duration ?? 0.0;
          if (offset < duration) {
            break;
          }
          offset -= duration;
          src.bufsProcessed++;
        }
        // 'offset' is now the intra-buffer offset within the target buffer.
        src.bufOffset = offset;
  
        if (playing) {
          AL.setSourceState(src, 4114);
        }
      },
  getGlobalParam:(funcname, param) => {
        if (!AL.currentCtx) {
          return null;
        }
  
        switch (param) {
        case 49152:
          return AL.currentCtx.dopplerFactor;
        case 49155:
          return AL.currentCtx.speedOfSound;
        case 53248:
          return AL.currentCtx.distanceModel;
        default:
          AL.currentCtx.err = 40962;
          return null;
        }
      },
  setGlobalParam:(funcname, param, value) => {
        if (!AL.currentCtx) {
          return;
        }
  
        switch (param) {
        case 49152:
          if (!Number.isFinite(value) || value < 0.0) { // Strictly negative values are disallowed
            AL.currentCtx.err = 40963;
            return;
          }
  
          AL.currentCtx.dopplerFactor = value;
          AL.updateListenerSpace(AL.currentCtx);
          break;
        case 49155:
          if (!Number.isFinite(value) || value <= 0.0) { // Negative or zero values are disallowed
            AL.currentCtx.err = 40963;
            return;
          }
  
          AL.currentCtx.speedOfSound = value;
          AL.updateListenerSpace(AL.currentCtx);
          break;
        case 53248:
          switch (value) {
          case 0:
          case 0xd001 /* AL_INVERSE_DISTANCE */:
          case 0xd002 /* AL_INVERSE_DISTANCE_CLAMPED */:
          case 0xd003 /* AL_LINEAR_DISTANCE */:
          case 0xd004 /* AL_LINEAR_DISTANCE_CLAMPED */:
          case 0xd005 /* AL_EXPONENT_DISTANCE */:
          case 0xd006 /* AL_EXPONENT_DISTANCE_CLAMPED */:
            AL.currentCtx.distanceModel = value;
            AL.updateContextGlobal(AL.currentCtx);
            break;
          default:
            AL.currentCtx.err = 40963;
            return;
          }
          break;
        default:
          AL.currentCtx.err = 40962;
          return;
        }
      },
  getListenerParam:(funcname, param) => {
        if (!AL.currentCtx) {
          return null;
        }
  
        switch (param) {
        case 4100:
          return AL.currentCtx.listener.position;
        case 4102:
          return AL.currentCtx.listener.velocity;
        case 4111:
          return AL.currentCtx.listener.direction.concat(AL.currentCtx.listener.up);
        case 4106:
          return AL.currentCtx.gain.gain.value;
        default:
          AL.currentCtx.err = 40962;
          return null;
        }
      },
  setListenerParam:(funcname, param, value) => {
        if (!AL.currentCtx) {
          return;
        }
        if (value === null) {
          AL.currentCtx.err = 40962;
          return;
        }
  
        var listener = AL.currentCtx.listener;
        switch (param) {
        case 4100:
          if (!Number.isFinite(value[0]) || !Number.isFinite(value[1]) || !Number.isFinite(value[2])) {
            AL.currentCtx.err = 40963;
            return;
          }
  
          listener.position[0] = value[0];
          listener.position[1] = value[1];
          listener.position[2] = value[2];
          AL.updateListenerSpace(AL.currentCtx);
          break;
        case 4102:
          if (!Number.isFinite(value[0]) || !Number.isFinite(value[1]) || !Number.isFinite(value[2])) {
            AL.currentCtx.err = 40963;
            return;
          }
  
          listener.velocity[0] = value[0];
          listener.velocity[1] = value[1];
          listener.velocity[2] = value[2];
          AL.updateListenerSpace(AL.currentCtx);
          break;
        case 4106:
          if (!Number.isFinite(value) || value < 0.0) {
            AL.currentCtx.err = 40963;
            return;
          }
  
          AL.currentCtx.gain.gain.value = value;
          break;
        case 4111:
          if (!Number.isFinite(value[0]) || !Number.isFinite(value[1]) || !Number.isFinite(value[2])
            || !Number.isFinite(value[3]) || !Number.isFinite(value[4]) || !Number.isFinite(value[5])
          ) {
            AL.currentCtx.err = 40963;
            return;
          }
  
          listener.direction[0] = value[0];
          listener.direction[1] = value[1];
          listener.direction[2] = value[2];
          listener.up[0] = value[3];
          listener.up[1] = value[4];
          listener.up[2] = value[5];
          AL.updateListenerSpace(AL.currentCtx);
          break;
        default:
          AL.currentCtx.err = 40962;
          return;
        }
      },
  getBufferParam:(funcname, bufferId, param) => {
        if (!AL.currentCtx) {
          return;
        }
        var buf = AL.buffers[bufferId];
        if (!buf || !bufferId) {
          AL.currentCtx.err = 40961;
          return;
        }
  
        switch (param) {
        case 0x2001 /* AL_FREQUENCY */:
          return buf.frequency;
        case 0x2002 /* AL_BITS */:
          return buf.bytesPerSample * 8;
        case 0x2003 /* AL_CHANNELS */:
          return buf.channels;
        case 0x2004 /* AL_SIZE */:
          return buf.length * buf.bytesPerSample * buf.channels;
        case 0x2015 /* AL_LOOP_POINTS_SOFT */:
          if (!buf.length) {
            return [0, 0];
          }
          return [
            (buf.audioBuf._loopStart ?? 0.0) * buf.frequency,
            (buf.audioBuf._loopEnd ?? buf.length) * buf.frequency
          ];
        default:
          AL.currentCtx.err = 40962;
          return null;
        }
      },
  setBufferParam:(funcname, bufferId, param, value) => {
        if (!AL.currentCtx) {
          return;
        }
        var buf = AL.buffers[bufferId];
        if (!buf || !bufferId) {
          AL.currentCtx.err = 40961;
          return;
        }
        if (value === null) {
          AL.currentCtx.err = 40962;
          return;
        }
  
        switch (param) {
        case 0x2004 /* AL_SIZE */:
          if (value) {
            AL.currentCtx.err = 40963;
            return;
          }
  
          // Per the spec, setting AL_SIZE to 0 is a legal NOP.
          break;
        case 0x2015 /* AL_LOOP_POINTS_SOFT */:
          if (value[0] < 0 || value[0] > buf.length || value[1] < 0 || value[1] > buf.Length || value[0] >= value[1]) {
            AL.currentCtx.err = 40963;
            return;
          }
          if (buf.refCount > 0) {
            AL.currentCtx.err = 40964;
            return;
          }
  
          if (buf.audioBuf) {
            buf.audioBuf._loopStart = value[0] / buf.frequency;
            buf.audioBuf._loopEnd = value[1] / buf.frequency;
          }
          break;
        default:
          AL.currentCtx.err = 40962;
          return;
        }
      },
  getSourceParam:(funcname, sourceId, param) => {
        if (!AL.currentCtx) {
          return null;
        }
        var src = AL.currentCtx.sources[sourceId];
        if (!src) {
          AL.currentCtx.err = 40961;
          return null;
        }
  
        switch (param) {
        case 0x202 /* AL_SOURCE_RELATIVE */:
          return src.relative;
        case 0x1001 /* AL_CONE_INNER_ANGLE */:
          return src.coneInnerAngle;
        case 0x1002 /* AL_CONE_OUTER_ANGLE */:
          return src.coneOuterAngle;
        case 0x1003 /* AL_PITCH */:
          return src.pitch;
        case 4100:
          return src.position;
        case 4101:
          return src.direction;
        case 4102:
          return src.velocity;
        case 0x1007 /* AL_LOOPING */:
          return src.looping;
        case 0x1009 /* AL_BUFFER */:
          if (src.type === 4136) {
            return src.bufQueue[0].id;
          }
          return 0;
        case 4106:
          return src.gain.gain.value;
         case 0x100D /* AL_MIN_GAIN */:
          return src.minGain;
        case 0x100E /* AL_MAX_GAIN */:
          return src.maxGain;
        case 0x1010 /* AL_SOURCE_STATE */:
          return src.state;
        case 0x1015 /* AL_BUFFERS_QUEUED */:
          if (src.bufQueue.length === 1 && !src.bufQueue[0].id) {
            return 0;
          }
          return src.bufQueue.length;
        case 0x1016 /* AL_BUFFERS_PROCESSED */:
          if ((src.bufQueue.length === 1 && !src.bufQueue[0].id) || src.looping) {
            return 0;
          }
          return src.bufsProcessed;
        case 0x1020 /* AL_REFERENCE_DISTANCE */:
          return src.refDistance;
        case 0x1021 /* AL_ROLLOFF_FACTOR */:
          return src.rolloffFactor;
        case 0x1022 /* AL_CONE_OUTER_GAIN */:
          return src.coneOuterGain;
        case 0x1023 /* AL_MAX_DISTANCE */:
          return src.maxDistance;
        case 0x1024 /* AL_SEC_OFFSET */:
          return AL.sourceTell(src);
        case 0x1025 /* AL_SAMPLE_OFFSET */:
          var offset = AL.sourceTell(src);
          if (offset > 0.0) {
            offset *= src.bufQueue[0].frequency;
          }
          return offset;
        case 0x1026 /* AL_BYTE_OFFSET */:
          var offset = AL.sourceTell(src);
          if (offset > 0.0) {
            offset *= src.bufQueue[0].frequency * src.bufQueue[0].bytesPerSample;
          }
          return offset;
        case 0x1027 /* AL_SOURCE_TYPE */:
          return src.type;
        case 0x1214 /* AL_SOURCE_SPATIALIZE_SOFT */:
          return src.spatialize;
        case 0x2009 /* AL_BYTE_LENGTH_SOFT */:
          var length = 0;
          var bytesPerFrame = 0;
          for (var buf of src.bufQueue) {
            length += buf.length;
            if (buf.id) {
              bytesPerFrame = buf.bytesPerSample * buf.channels;
            }
          }
          return length * bytesPerFrame;
        case 0x200A /* AL_SAMPLE_LENGTH_SOFT */:
          var length = 0;
          for (var buf of src.bufQueue) {
            length += buf.length;
          }
          return length;
        case 0x200B /* AL_SEC_LENGTH_SOFT */:
          return AL.sourceDuration(src);
        case 53248:
          return src.distanceModel;
        default:
          AL.currentCtx.err = 40962;
          return null;
        }
      },
  setSourceParam:(funcname, sourceId, param, value) => {
        if (!AL.currentCtx) {
          return;
        }
        var src = AL.currentCtx.sources[sourceId];
        if (!src) {
          AL.currentCtx.err = 40961;
          return;
        }
        if (value === null) {
          AL.currentCtx.err = 40962;
          return;
        }
  
        switch (param) {
        case 0x202 /* AL_SOURCE_RELATIVE */:
          if (value === 1) {
            src.relative = true;
            AL.updateSourceSpace(src);
          } else if (value === 0) {
            src.relative = false;
            AL.updateSourceSpace(src);
          } else {
            AL.currentCtx.err = 40963;
            return;
          }
          break;
        case 0x1001 /* AL_CONE_INNER_ANGLE */:
          if (!Number.isFinite(value)) {
            AL.currentCtx.err = 40963;
            return;
          }
  
          src.coneInnerAngle = value;
          if (src.panner) {
            src.panner.coneInnerAngle = value % 360.0;
          }
          break;
        case 0x1002 /* AL_CONE_OUTER_ANGLE */:
          if (!Number.isFinite(value)) {
            AL.currentCtx.err = 40963;
            return;
          }
  
          src.coneOuterAngle = value;
          if (src.panner) {
            src.panner.coneOuterAngle = value % 360.0;
          }
          break;
        case 0x1003 /* AL_PITCH */:
          if (!Number.isFinite(value) || value <= 0.0) {
            AL.currentCtx.err = 40963;
            return;
          }
  
          if (src.pitch === value) {
            break;
          }
  
          src.pitch = value;
          AL.updateSourceRate(src);
          break;
        case 4100:
          if (!Number.isFinite(value[0]) || !Number.isFinite(value[1]) || !Number.isFinite(value[2])) {
            AL.currentCtx.err = 40963;
            return;
          }
  
          src.position[0] = value[0];
          src.position[1] = value[1];
          src.position[2] = value[2];
          AL.updateSourceSpace(src);
          break;
        case 4101:
          if (!Number.isFinite(value[0]) || !Number.isFinite(value[1]) || !Number.isFinite(value[2])) {
            AL.currentCtx.err = 40963;
            return;
          }
  
          src.direction[0] = value[0];
          src.direction[1] = value[1];
          src.direction[2] = value[2];
          AL.updateSourceSpace(src);
          break;
        case 4102:
          if (!Number.isFinite(value[0]) || !Number.isFinite(value[1]) || !Number.isFinite(value[2])) {
            AL.currentCtx.err = 40963;
            return;
          }
  
          src.velocity[0] = value[0];
          src.velocity[1] = value[1];
          src.velocity[2] = value[2];
          AL.updateSourceSpace(src);
          break;
        case 0x1007 /* AL_LOOPING */:
          if (value === 1) {
            src.looping = true;
            AL.updateSourceTime(src);
            if (src.type === 4136 && src.audioQueue.length > 0) {
              var audioSrc  = src.audioQueue[0];
              audioSrc.loop = true;
              audioSrc._duration = Number.POSITIVE_INFINITY;
            }
          } else if (value === 0) {
            src.looping = false;
            var currentTime = AL.updateSourceTime(src);
            if (src.type === 4136 && src.audioQueue.length > 0) {
              var audioSrc  = src.audioQueue[0];
              audioSrc.loop = false;
              audioSrc._duration = src.bufQueue[0].audioBuf.duration / src.playbackRate;
              audioSrc._startTime = currentTime - src.bufOffset / src.playbackRate;
            }
          } else {
            AL.currentCtx.err = 40963;
            return;
          }
          break;
        case 0x1009 /* AL_BUFFER */:
          if (src.state === 4114 || src.state === 4115) {
            AL.currentCtx.err = 40964;
            return;
          }
  
          var buf = AL.buffers[value];
          if (!buf) {
            AL.currentCtx.err = 40963;
            return;
          }
  
          for (var oldBuf of src.bufQueue) {
            oldBuf.refCount--;
          }
  
          src.bufQueue = [buf];
          src.bufsProcessed = 0;
  
          if (!value) {
            src.type = 0x1030 /* AL_UNDETERMINED */;
          } else {
            buf.refCount++;
            src.type = 4136;
          }
  
          AL.initSourcePanner(src);
          AL.scheduleSourceAudio(src);
          break;
        case 4106:
          if (!Number.isFinite(value) || value < 0.0) {
            AL.currentCtx.err = 40963;
            return;
          }
          src.gain.gain.value = value;
          break;
        case 0x100D /* AL_MIN_GAIN */:
          if (!Number.isFinite(value) || value < 0.0 || value > Math.min(src.maxGain, 1.0)) {
            AL.currentCtx.err = 40963;
            return;
          }
          src.minGain = value;
          break;
        case 0x100E /* AL_MAX_GAIN */:
          if (!Number.isFinite(value) || value < Math.max(0.0, src.minGain) || value > 1.0) {
            AL.currentCtx.err = 40963;
            return;
          }
          src.maxGain = value;
          break;
        case 0x1020 /* AL_REFERENCE_DISTANCE */:
          if (!Number.isFinite(value) || value < 0.0) {
            AL.currentCtx.err = 40963;
            return;
          }
          src.refDistance = value;
          if (src.panner) {
            src.panner.refDistance = value;
          }
          break;
        case 0x1021 /* AL_ROLLOFF_FACTOR */:
          if (!Number.isFinite(value) || value < 0.0) {
            AL.currentCtx.err = 40963;
            return;
          }
          src.rolloffFactor = value;
          if (src.panner) {
            src.panner.rolloffFactor = value;
          }
          break;
        case 0x1022 /* AL_CONE_OUTER_GAIN */:
          if (!Number.isFinite(value) || value < 0.0 || value > 1.0) {
            AL.currentCtx.err = 40963;
            return;
          }
          src.coneOuterGain = value;
          if (src.panner) {
            src.panner.coneOuterGain = value;
          }
          break;
        case 0x1023 /* AL_MAX_DISTANCE */:
          if (!Number.isFinite(value) || value < 0.0) {
            AL.currentCtx.err = 40963;
            return;
          }
          src.maxDistance = value;
          if (src.panner) {
            src.panner.maxDistance = value;
          }
          break;
        case 0x1024 /* AL_SEC_OFFSET */:
          if (value < 0.0 || value > AL.sourceDuration(src)) {
            AL.currentCtx.err = 40963;
            return;
          }
  
          AL.sourceSeek(src, value);
          break;
        case 0x1025 /* AL_SAMPLE_OFFSET */:
          var srcLen = AL.sourceDuration(src);
          if (srcLen > 0.0) {
            var frequency;
            for (var buf of src.bufQueue) {
              if (buf.id) {
                frequency = buf.frequency;
                break;
              }
            }
            value /= frequency;
          }
          if (value < 0.0 || value > srcLen) {
            AL.currentCtx.err = 40963;
            return;
          }
  
          AL.sourceSeek(src, value);
          break;
        case 0x1026 /* AL_BYTE_OFFSET */:
          var srcLen = AL.sourceDuration(src);
          if (srcLen > 0.0) {
            var bytesPerSec;
            for (var buf of src.bufQueue) {
              if (buf.id) {
                bytesPerSec = buf.frequency * buf.bytesPerSample * buf.channels;
                break;
              }
            }
            value /= bytesPerSec;
          }
          if (value < 0.0 || value > srcLen) {
            AL.currentCtx.err = 40963;
            return;
          }
  
          AL.sourceSeek(src, value);
          break;
        case 0x1214 /* AL_SOURCE_SPATIALIZE_SOFT */:
          if (value !== 0 && value !== 1 && value !== 2 /* AL_AUTO_SOFT */) {
            AL.currentCtx.err = 40963;
            return;
          }
  
          src.spatialize = value;
          AL.initSourcePanner(src);
          break;
        case 0x2009 /* AL_BYTE_LENGTH_SOFT */:
        case 0x200A /* AL_SAMPLE_LENGTH_SOFT */:
        case 0x200B /* AL_SEC_LENGTH_SOFT */:
          AL.currentCtx.err = 40964;
          break;
        case 53248:
          switch (value) {
          case 0:
          case 0xd001 /* AL_INVERSE_DISTANCE */:
          case 0xd002 /* AL_INVERSE_DISTANCE_CLAMPED */:
          case 0xd003 /* AL_LINEAR_DISTANCE */:
          case 0xd004 /* AL_LINEAR_DISTANCE_CLAMPED */:
          case 0xd005 /* AL_EXPONENT_DISTANCE */:
          case 0xd006 /* AL_EXPONENT_DISTANCE_CLAMPED */:
            src.distanceModel = value;
            if (AL.currentCtx.sourceDistanceModel) {
              AL.updateContextGlobal(AL.currentCtx);
            }
            break;
          default:
            AL.currentCtx.err = 40963;
            return;
          }
          break;
        default:
          AL.currentCtx.err = 40962;
          return;
        }
      },
  captures:{
  },
  sharedCaptureAudioCtx:null,
  requireValidCaptureDevice:(deviceId, funcname) => {
        if (!deviceId) {
          AL.alcErr = 40961;
          return null;
        }
        var c = AL.captures[deviceId];
        if (!c) {
          AL.alcErr = 40961;
          return null;
        }
        var err = c.mediaStreamError;
        if (err) {
          AL.alcErr = 40961;
          return null;
        }
        return c;
      },
  };
  
  
  
  /** @type {!Float32Array} */
  var HEAPF32;
  var _alBufferData = (bufferId, format, pData, size, freq) => {
      if (!AL.currentCtx) {
        return;
      }
      var buf = AL.buffers[bufferId];
      if (!buf) {
        AL.currentCtx.err = 40963;
        return;
      }
      if (freq <= 0) {
        AL.currentCtx.err = 40963;
        return;
      }
  
      var audioBuf = null;
      try {
        switch (format) {
        case 0x1100 /* AL_FORMAT_MONO8 */:
          if (size > 0) {
            audioBuf = AL.currentCtx.audioCtx.createBuffer(1, size, freq);
            var channel0 = audioBuf.getChannelData(0);
            for (var i = 0; i < size; ++i) {
              channel0[i] = HEAPU8[pData++] * 0.0078125 /* 1/128 */ - 1.0;
            }
          }
          buf.bytesPerSample = 1;
          buf.channels = 1;
          buf.length = size;
          break;
        case 0x1101 /* AL_FORMAT_MONO16 */:
          if (size > 0) {
            audioBuf = AL.currentCtx.audioCtx.createBuffer(1, size >> 1, freq);
            var channel0 = audioBuf.getChannelData(0);
            pData >>= 1;
            for (var i = 0; i < size >> 1; ++i) {
              channel0[i] = HEAP16[pData++] * 0.000030517578125 /* 1/32768 */;
            }
          }
          buf.bytesPerSample = 2;
          buf.channels = 1;
          buf.length = size >> 1;
          break;
        case 0x1102 /* AL_FORMAT_STEREO8 */:
          if (size > 0) {
            audioBuf = AL.currentCtx.audioCtx.createBuffer(2, size >> 1, freq);
            var channel0 = audioBuf.getChannelData(0);
            var channel1 = audioBuf.getChannelData(1);
            for (var i = 0; i < size >> 1; ++i) {
              channel0[i] = HEAPU8[pData++] * 0.0078125 /* 1/128 */ - 1.0;
              channel1[i] = HEAPU8[pData++] * 0.0078125 /* 1/128 */ - 1.0;
            }
          }
          buf.bytesPerSample = 1;
          buf.channels = 2;
          buf.length = size >> 1;
          break;
        case 0x1103 /* AL_FORMAT_STEREO16 */:
          if (size > 0) {
            audioBuf = AL.currentCtx.audioCtx.createBuffer(2, size >> 2, freq);
            var channel0 = audioBuf.getChannelData(0);
            var channel1 = audioBuf.getChannelData(1);
            pData >>= 1;
            for (var i = 0; i < size >> 2; ++i) {
              channel0[i] = HEAP16[pData++] * 0.000030517578125 /* 1/32768 */;
              channel1[i] = HEAP16[pData++] * 0.000030517578125 /* 1/32768 */;
            }
          }
          buf.bytesPerSample = 2;
          buf.channels = 2;
          buf.length = size >> 2;
          break;
        case 0x10010 /* AL_FORMAT_MONO_FLOAT32 */:
          if (size > 0) {
            audioBuf = AL.currentCtx.audioCtx.createBuffer(1, size >> 2, freq);
            var channel0 = audioBuf.getChannelData(0);
            pData >>= 2;
            for (var i = 0; i < size >> 2; ++i) {
              channel0[i] = HEAPF32[pData++];
            }
          }
          buf.bytesPerSample = 4;
          buf.channels = 1;
          buf.length = size >> 2;
          break;
        case 0x10011 /* AL_FORMAT_STEREO_FLOAT32 */:
          if (size > 0) {
            audioBuf = AL.currentCtx.audioCtx.createBuffer(2, size >> 3, freq);
            var channel0 = audioBuf.getChannelData(0);
            var channel1 = audioBuf.getChannelData(1);
            pData >>= 2;
            for (var i = 0; i < size >> 3; ++i) {
              channel0[i] = HEAPF32[pData++];
              channel1[i] = HEAPF32[pData++];
            }
          }
          buf.bytesPerSample = 4;
          buf.channels = 2;
          buf.length = size >> 3;
          break;
        default:
          AL.currentCtx.err = 40963;
          return;
        }
        buf.frequency = freq;
        buf.audioBuf = audioBuf;
      } catch (e) {
        AL.currentCtx.err = 40963;
        return;
      }
    };

  
  var _alBufferiv = (bufferId, param, pValues) => {
      if (!AL.currentCtx) {
        return;
      }
      if (!pValues) {
        AL.currentCtx.err = 40963;
        return;
      }
  
      switch (param) {
      case 0x2015 /* AL_LOOP_POINTS_SOFT */:
        AL.paramArray[0] = HEAP32[((pValues)>>2)];
        AL.paramArray[1] = HEAP32[(((pValues)+(4))>>2)];
        AL.setBufferParam('alBufferiv', bufferId, param, AL.paramArray);
        break;
      default:
        AL.setBufferParam('alBufferiv', bufferId, param, null);
        break;
      }
    };

  
  var _alDeleteBuffers = (count, pBufferIds) => {
      if (!AL.currentCtx) {
        return;
      }
  
      for (var i = 0; i < count; ++i) {
        var bufId = HEAP32[(((pBufferIds)+(i*4))>>2)];
        // Deleting the zero buffer is a legal NOP, so ignore it
        if (!bufId) {
          continue;
        }
  
        // Make sure the buffer index is valid.
        if (!AL.buffers[bufId]) {
          AL.currentCtx.err = 40961;
          return;
        }
  
        // Make sure the buffer is no longer in use.
        if (AL.buffers[bufId].refCount) {
          AL.currentCtx.err = 40964;
          return;
        }
      }
  
      for (var i = 0; i < count; ++i) {
        var bufId = HEAP32[(((pBufferIds)+(i*4))>>2)];
        if (!bufId) {
          continue;
        }
  
        AL.deviceRefCounts[AL.buffers[bufId].deviceId]--;
        delete AL.buffers[bufId];
        AL.freeIds.push(bufId);
      }
    };

  var _alSourcei = (sourceId, param, value) => {
      switch (param) {
      case 0x202 /* AL_SOURCE_RELATIVE */:
      case 0x1001 /* AL_CONE_INNER_ANGLE */:
      case 0x1002 /* AL_CONE_OUTER_ANGLE */:
      case 0x1007 /* AL_LOOPING */:
      case 0x1009 /* AL_BUFFER */:
      case 0x1020 /* AL_REFERENCE_DISTANCE */:
      case 0x1021 /* AL_ROLLOFF_FACTOR */:
      case 0x1023 /* AL_MAX_DISTANCE */:
      case 0x1024 /* AL_SEC_OFFSET */:
      case 0x1025 /* AL_SAMPLE_OFFSET */:
      case 0x1026 /* AL_BYTE_OFFSET */:
      case 0x1214 /* AL_SOURCE_SPATIALIZE_SOFT */:
      case 0x2009 /* AL_BYTE_LENGTH_SOFT */:
      case 0x200A /* AL_SAMPLE_LENGTH_SOFT */:
      case 53248:
        AL.setSourceParam('alSourcei', sourceId, param, value);
        break;
      default:
        AL.setSourceParam('alSourcei', sourceId, param, null);
        break;
      }
    };
  
  
  var _alDeleteSources = (count, pSourceIds) => {
      if (!AL.currentCtx) {
        return;
      }
  
      for (var i = 0; i < count; ++i) {
        var srcId = HEAP32[(((pSourceIds)+(i*4))>>2)];
        if (!AL.currentCtx.sources[srcId]) {
          AL.currentCtx.err = 40961;
          return;
        }
      }
  
      for (var i = 0; i < count; ++i) {
        var srcId = HEAP32[(((pSourceIds)+(i*4))>>2)];
        AL.setSourceState(AL.currentCtx.sources[srcId], 4116);
        _alSourcei(srcId, 0x1009 /* AL_BUFFER */, 0);
        delete AL.currentCtx.sources[srcId];
        AL.freeIds.push(srcId);
      }
    };

  var _alDistanceModel = (model) => {
      AL.setGlobalParam('alDistanceModel', 53248, model);
    };

  
  var _alGenBuffers = (count, pBufferIds) => {
      if (!AL.currentCtx) {
        return;
      }
  
      for (var i = 0; i < count; ++i) {
        var buf = {
          deviceId: AL.currentCtx.deviceId,
          id: AL.newId(),
          refCount: 0,
          audioBuf: null,
          frequency: 0,
          bytesPerSample: 2,
          channels: 1,
          length: 0,
        };
        AL.deviceRefCounts[buf.deviceId]++;
        AL.buffers[buf.id] = buf;
        HEAP32[(((pBufferIds)+(i*4))>>2)] = buf.id;
      }
    };

  
  var _alGenSources = (count, pSourceIds) => {
      if (!AL.currentCtx) {
        return;
      }
      for (var i = 0; i < count; ++i) {
        var gain = AL.currentCtx.audioCtx.createGain();
        gain.connect(AL.currentCtx.gain);
        var src = {
          context: AL.currentCtx,
          id: AL.newId(),
          type: 0x1030 /* AL_UNDETERMINED */,
          state: 4113,
          bufQueue: [AL.buffers[0]],
          audioQueue: [],
          looping: false,
          pitch: 1.0,
          dopplerShift: 1.0,
          gain,
          minGain: 0.0,
          maxGain: 1.0,
          panner: null,
          bufsProcessed: 0,
          bufStartTime: Number.NEGATIVE_INFINITY,
          bufOffset: 0.0,
          relative: false,
          refDistance: 1.0,
          maxDistance: 3.40282e38 /* FLT_MAX */,
          rolloffFactor: 1.0,
          position: [0.0, 0.0, 0.0],
          velocity: [0.0, 0.0, 0.0],
          direction: [0.0, 0.0, 0.0],
          coneOuterGain: 0.0,
          coneInnerAngle: 360.0,
          coneOuterAngle: 360.0,
          distanceModel: 0xd002 /* AL_INVERSE_DISTANCE_CLAMPED */,
          spatialize: 2 /* AL_AUTO_SOFT */,
  
          get playbackRate() {
            return this.pitch * this.dopplerShift;
          }
        };
        AL.currentCtx.sources[src.id] = src;
        HEAP32[(((pSourceIds)+(i*4))>>2)] = src.id;
      }
    };

  
  var _alGetSourcei = (sourceId, param, pValue) => {
      var val = AL.getSourceParam('alGetSourcei', sourceId, param);
      if (val === null) {
        return;
      }
      if (!pValue) {
        AL.currentCtx.err = 40963;
        return;
      }
  
      switch (param) {
      case 0x202 /* AL_SOURCE_RELATIVE */:
      case 0x1001 /* AL_CONE_INNER_ANGLE */:
      case 0x1002 /* AL_CONE_OUTER_ANGLE */:
      case 0x1007 /* AL_LOOPING */:
      case 0x1009 /* AL_BUFFER */:
      case 0x1010 /* AL_SOURCE_STATE */:
      case 0x1015 /* AL_BUFFERS_QUEUED */:
      case 0x1016 /* AL_BUFFERS_PROCESSED */:
      case 0x1020 /* AL_REFERENCE_DISTANCE */:
      case 0x1021 /* AL_ROLLOFF_FACTOR */:
      case 0x1023 /* AL_MAX_DISTANCE */:
      case 0x1024 /* AL_SEC_OFFSET */:
      case 0x1025 /* AL_SAMPLE_OFFSET */:
      case 0x1026 /* AL_BYTE_OFFSET */:
      case 0x1027 /* AL_SOURCE_TYPE */:
      case 0x1214 /* AL_SOURCE_SPATIALIZE_SOFT */:
      case 0x2009 /* AL_BYTE_LENGTH_SOFT */:
      case 0x200A /* AL_SAMPLE_LENGTH_SOFT */:
      case 53248:
        HEAP32[((pValue)>>2)] = val;
        break;
      default:
        AL.currentCtx.err = 40962;
        return;
      }
    };

  
  
  var stringToNewUTF8 = (str) => {
      var size = lengthBytesUTF8(str) + 1;
      var ret = _malloc(size);
      if (ret) stringToUTF8(str, ret, size);
      return ret;
    };
  
  var _alGetString = (param) => {
      if (AL.stringCache[param]) {
        return AL.stringCache[param];
      }
  
      var ret;
      switch (param) {
      case 0:
        ret = 'No Error';
        break;
      case 40961:
        ret = 'Invalid Name';
        break;
      case 40962:
        ret = 'Invalid Enum';
        break;
      case 40963:
        ret = 'Invalid Value';
        break;
      case 40964:
        ret = 'Invalid Operation';
        break;
      case 0xA005 /* AL_OUT_OF_MEMORY */:
        ret = 'Out of Memory';
        break;
      case 0xB001 /* AL_VENDOR */:
        ret = 'Emscripten';
        break;
      case 0xB002 /* AL_VERSION */:
        ret = '1.1';
        break;
      case 0xB003 /* AL_RENDERER */:
        ret = 'WebAudio';
        break;
      case 0xB004 /* AL_EXTENSIONS */:
        ret = Object.keys(AL.AL_EXTENSIONS).join(' ');
        break;
      default:
        if (AL.currentCtx) {
          AL.currentCtx.err = 40962;
        } else {
        }
        return 0;
      }
  
      ret = stringToNewUTF8(ret);
      AL.stringCache[param] = ret;
      return ret;
    };

  var _alIsBuffer = (bufferId) => {
      if (!AL.currentCtx) {
        return false;
      }
      if (bufferId > AL.buffers.length) {
        return false;
      }
  
      if (!AL.buffers[bufferId]) {
        return false;
      }
      return true;
    };

  var _alListener3f = (param, value0, value1, value2) => {
      switch (param) {
      case 4100:
      case 4102:
        AL.paramArray[0] = value0;
        AL.paramArray[1] = value1;
        AL.paramArray[2] = value2;
        AL.setListenerParam('alListener3f', param, AL.paramArray);
        break;
      default:
        AL.setListenerParam('alListener3f', param, null);
        break;
      }
    };

  var _alListenerf = (param, value) => {
      switch (param) {
      case 4106:
        AL.setListenerParam('alListenerf', param, value);
        break;
      default:
        AL.setListenerParam('alListenerf', param, null);
        break;
      }
    };

  
  var _alListenerfv = (param, pValues) => {
      if (!AL.currentCtx) {
        return;
      }
      if (!pValues) {
        AL.currentCtx.err = 40963;
        return;
      }
  
      switch (param) {
      case 4100:
      case 4102:
        AL.paramArray[0] = HEAPF32[((pValues)>>2)];
        AL.paramArray[1] = HEAPF32[(((pValues)+(4))>>2)];
        AL.paramArray[2] = HEAPF32[(((pValues)+(8))>>2)];
        AL.setListenerParam('alListenerfv', param, AL.paramArray);
        break;
      case 4111:
        AL.paramArray[0] = HEAPF32[((pValues)>>2)];
        AL.paramArray[1] = HEAPF32[(((pValues)+(4))>>2)];
        AL.paramArray[2] = HEAPF32[(((pValues)+(8))>>2)];
        AL.paramArray[3] = HEAPF32[(((pValues)+(12))>>2)];
        AL.paramArray[4] = HEAPF32[(((pValues)+(16))>>2)];
        AL.paramArray[5] = HEAPF32[(((pValues)+(20))>>2)];
        AL.setListenerParam('alListenerfv', param, AL.paramArray);
        break;
      default:
        AL.setListenerParam('alListenerfv', param, null);
        break;
      }
    };

  var _alSource3f = (sourceId, param, value0, value1, value2) => {
      switch (param) {
      case 4100:
      case 4101:
      case 4102:
        AL.paramArray[0] = value0;
        AL.paramArray[1] = value1;
        AL.paramArray[2] = value2;
        AL.setSourceParam('alSource3f', sourceId, param, AL.paramArray);
        break;
      default:
        AL.setSourceParam('alSource3f', sourceId, param, null);
        break;
      }
    };

  var _alSource3i = (sourceId, param, value0, value1, value2) => {
      switch (param) {
      case 4100:
      case 4101:
      case 4102:
        AL.paramArray[0] = value0;
        AL.paramArray[1] = value1;
        AL.paramArray[2] = value2;
        AL.setSourceParam('alSource3i', sourceId, param, AL.paramArray);
        break;
      default:
        AL.setSourceParam('alSource3i', sourceId, param, null);
        break;
      }
    };

  var _alSourcePause = (sourceId) => {
      if (!AL.currentCtx) {
        return;
      }
      var src = AL.currentCtx.sources[sourceId];
      if (!src) {
        AL.currentCtx.err = 40961;
        return;
      }
      AL.setSourceState(src, 4115);
    };

  var _alSourcePlay = (sourceId) => {
      if (!AL.currentCtx) {
        return;
      }
      var src = AL.currentCtx.sources[sourceId];
      if (!src) {
        AL.currentCtx.err = 40961;
        return;
      }
      AL.setSourceState(src, 4114);
    };

  
  var _alSourceQueueBuffers = (sourceId, count, pBufferIds) => {
      if (!AL.currentCtx) {
        return;
      }
      var src = AL.currentCtx.sources[sourceId];
      if (!src) {
        AL.currentCtx.err = 40961;
        return;
      }
      if (src.type === 4136) {
        AL.currentCtx.err = 40964;
        return;
      }
  
      if (!count) {
        return;
      }
  
      // Find the first non-zero buffer in the queue to determine the proper format
      var templateBuf = AL.buffers[0];
      for (var buf of src.bufQueue) {
        if (buf.id) {
          templateBuf = buf;
          break;
        }
      }
  
      for (var i = 0; i < count; ++i) {
        var bufId = HEAP32[(((pBufferIds)+(i*4))>>2)];
        var buf = AL.buffers[bufId];
        if (!buf) {
          AL.currentCtx.err = 40961;
          return;
        }
  
        // Check that the added buffer has the correct format. If the template is the zero buffer, any format is valid.
        if (templateBuf.id && (
          buf.frequency !== templateBuf.frequency
          || buf.bytesPerSample !== templateBuf.bytesPerSample
          || buf.channels !== templateBuf.channels)
        ) {
          AL.currentCtx.err = 40964;
        }
      }
  
      // If the only buffer in the queue is the zero buffer, clear the queue before we add anything.
      if (src.bufQueue.length === 1 && !src.bufQueue[0].id) {
        src.bufQueue.length = 0;
      }
  
      src.type = 0x1029 /* AL_STREAMING */;
      for (var i = 0; i < count; ++i) {
        var bufId = HEAP32[(((pBufferIds)+(i*4))>>2)];
        var buf = AL.buffers[bufId];
        buf.refCount++;
        src.bufQueue.push(buf);
      }
  
      // if the source is looping, cancel the schedule so we can reschedule the loop order
      if (src.looping) {
        AL.cancelPendingSourceAudio(src);
      }
  
      AL.initSourcePanner(src);
      AL.scheduleSourceAudio(src);
    };

  var _alSourceStop = (sourceId) => {
      if (!AL.currentCtx) {
        return;
      }
      var src = AL.currentCtx.sources[sourceId];
      if (!src) {
        AL.currentCtx.err = 40961;
        return;
      }
      AL.setSourceState(src, 4116);
    };

  
  var _alSourceUnqueueBuffers = (sourceId, count, pBufferIds) => {
      if (!AL.currentCtx) {
        return;
      }
      var src = AL.currentCtx.sources[sourceId];
      if (!src) {
        AL.currentCtx.err = 40961;
        return;
      }
      if (count > (src.bufQueue.length === 1 && !src.bufQueue[0].id ? 0 : src.bufsProcessed)) {
        AL.currentCtx.err = 40963;
        return;
      }
  
      if (!count) {
        return;
      }
  
      for (var i = 0; i < count; i++) {
        var buf = src.bufQueue.shift();
        buf.refCount--;
        // Write the buffers index out to the return list.
        HEAP32[(((pBufferIds)+(i*4))>>2)] = buf.id;
        src.bufsProcessed--;
      }
  
      /// If the queue is empty, put the zero buffer back in
      if (!src.bufQueue.length) {
        src.bufQueue.push(AL.buffers[0]);
      }
  
      AL.initSourcePanner(src);
      AL.scheduleSourceAudio(src);
    };

  var _alSourcef = (sourceId, param, value) => {
      switch (param) {
      case 0x1001 /* AL_CONE_INNER_ANGLE */:
      case 0x1002 /* AL_CONE_OUTER_ANGLE */:
      case 0x1003 /* AL_PITCH */:
      case 4106:
      case 0x100D /* AL_MIN_GAIN */:
      case 0x100E /* AL_MAX_GAIN */:
      case 0x1020 /* AL_REFERENCE_DISTANCE */:
      case 0x1021 /* AL_ROLLOFF_FACTOR */:
      case 0x1022 /* AL_CONE_OUTER_GAIN */:
      case 0x1023 /* AL_MAX_DISTANCE */:
      case 0x1024 /* AL_SEC_OFFSET */:
      case 0x1025 /* AL_SAMPLE_OFFSET */:
      case 0x1026 /* AL_BYTE_OFFSET */:
      case 0x200B /* AL_SEC_LENGTH_SOFT */:
        AL.setSourceParam('alSourcef', sourceId, param, value);
        break;
      default:
        AL.setSourceParam('alSourcef', sourceId, param, null);
        break;
      }
    };


  var _alcCloseDevice = (deviceId) => {
      if (!(deviceId in AL.deviceRefCounts) || AL.deviceRefCounts[deviceId] > 0) {
        return 0;
      }
  
      delete AL.deviceRefCounts[deviceId];
      AL.freeIds.push(deviceId);
      return 1;
    };

  var autoResumeAudioContext = (ctx) => {
      for (var event of ['keydown', 'mousedown', 'touchstart']) {
        for (var element of [document, document.getElementById('canvas')]) {
          element?.addEventListener(event, () => {
            if (ctx.state === 'suspended') ctx.resume();
          }, { 'once': true });
        }
      }
    };
  
  
  var _alcCreateContext = (deviceId, pAttrList) => {
      if (!(deviceId in AL.deviceRefCounts)) {
        AL.alcErr = 0xA001; /* ALC_INVALID_DEVICE */
        return 0;
      }
  
      var options = {};
      var attrs = [];
      var hrtf = null;
      pAttrList >>= 2;
      if (pAttrList) {
        var attr = 0;
        var val = 0;
        while (true) {
          attr = HEAP32[pAttrList++];
          attrs.push(attr);
          if (!attr) {
            break;
          }
          val = HEAP32[pAttrList++];
          attrs.push(val);
  
          switch (attr) {
          case 0x1007 /* ALC_FREQUENCY */:
            options.sampleRate = val;
            break;
          case 0x1010 /* ALC_MONO_SOURCES */: // fallthrough
          case 0x1011 /* ALC_STEREO_SOURCES */:
            // Do nothing; these hints are satisfied by default
            break
          case 0x1992 /* ALC_HRTF_SOFT */:
            switch (val) {
              case 0:
                hrtf = false;
                break;
              case 1:
                hrtf = true;
                break;
              case 2 /* ALC_DONT_CARE_SOFT */:
                break;
              default:
                AL.alcErr = 40964;
                return 0;
            }
            break;
          case 0x1996 /* ALC_HRTF_ID_SOFT */:
            if (val) {
              AL.alcErr = 40964;
              return 0;
            }
            break;
          default:
            AL.alcErr = 0xA004; /* ALC_INVALID_VALUE */
            return 0;
          }
        }
      }
  
      var ac;
      try {
        ac = new AudioContext(options);
      } catch (e) {
        if (e.name === 'NotSupportedError') {
          AL.alcErr = 0xA004; /* ALC_INVALID_VALUE */
        } else {
          AL.alcErr = 0xA001; /* ALC_INVALID_DEVICE */
        }
  
        return 0;
      }
  
      autoResumeAudioContext(ac);
  
      // Old Web Audio API (e.g. Safari 6.0.5) had an inconsistently named createGainNode function.
      if (typeof ac.createGain == 'undefined') {
        ac.createGain = ac.createGainNode;
      }
  
      var gain = ac.createGain();
      gain.connect(ac.destination);
      var ctx = {
        deviceId,
        id: AL.newId(),
        attrs,
        audioCtx: ac,
        listener: {
          position: [0.0, 0.0, 0.0],
          velocity: [0.0, 0.0, 0.0],
          direction: [0.0, 0.0, 0.0],
          up: [0.0, 0.0, 0.0]
        },
        sources: [],
        interval: setInterval(() => AL.scheduleContextAudio(ctx), AL.QUEUE_INTERVAL),
        gain,
        distanceModel: 0xd002 /* AL_INVERSE_DISTANCE_CLAMPED */,
        speedOfSound: 343.3,
        dopplerFactor: 1.0,
        sourceDistanceModel: false,
        hrtf: hrtf || false,
  
        _err: 0,
        get err() {
          return this._err;
        },
        set err(val) {
          // Errors should not be overwritten by later errors until they are cleared by a query.
          if (this._err === 0 || val === 0) {
            this._err = val;
          }
        }
      };
      AL.deviceRefCounts[deviceId]++;
      AL.contexts[ctx.id] = ctx;
  
      if (hrtf !== null) {
        // Apply hrtf attrib to all contexts for this device
        for (var ctxId in AL.contexts) {
          var c = AL.contexts[ctxId];
          if (c.deviceId === deviceId) {
            c.hrtf = hrtf;
            AL.updateContextGlobal(c);
          }
        }
      }
  
      return ctx.id;
    };

  var _alcDestroyContext = (contextId) => {
      var ctx = AL.contexts[contextId];
      if (AL.currentCtx === ctx) {
        AL.alcErr = 0xA002 /* ALC_INVALID_CONTEXT */;
        return;
      }
  
      // Stop playback, etc
      if (AL.contexts[contextId].interval) {
        clearInterval(AL.contexts[contextId].interval);
      }
      AL.deviceRefCounts[ctx.deviceId]--;
      delete AL.contexts[contextId];
      AL.freeIds.push(contextId);
    };

  
  var _alcIsExtensionPresent = (deviceId, pExtName) => {
      var name = UTF8ToString(pExtName);
  
      return AL.ALC_EXTENSIONS[name] ? 1 : 0;
    };

  var _alcMakeContextCurrent = (contextId) => {
      AL.currentCtx = AL.contexts[contextId];
      return 1;
    };

  
  var _alcOpenDevice = (pDeviceName) => {
      if (pDeviceName) {
        var name = UTF8ToString(pDeviceName);
        if (name !== AL.DEVICE_NAME) {
          return 0;
        }
      }
  
      if (!globalThis.AudioContext) {
        return 0;
      }
  
      var deviceId = AL.newId();
      AL.deviceRefCounts[deviceId] = 0;
      return deviceId;
    };

  var _alcSuspendContext = (contextId) => {};

  
  var _emscripten_date_now = () => Date.now();
  
  var nowIsMonotonic = 1;
  
  var checkWasiClock = (clock_id) => clock_id >= 0 && clock_id <= 3;
  
  
  function _clock_time_get(clk_id, ignored_precision, ptime) {
    ignored_precision = bigintToI53Checked(ignored_precision);
  
  
      if (!checkWasiClock(clk_id)) {
        return 28;
      }
      var now;
      // all wasi clocks but realtime are monotonic
      if (clk_id === 0) {
        now = _emscripten_date_now();
      } else if (nowIsMonotonic) {
        now = _emscripten_get_now();
      } else {
        return 52;
      }
      // "now" is in ms, and wasi times are in ns.
      var nsec = Math.round(now * 1000 * 1000);
      HEAP64[((ptime)>>3)] = BigInt(nsec);
      return 0;
    ;
  }

  var readEmAsmArgsArray = [];
  
  
  
  
  /** @type {!Float64Array} */
  var HEAPF64;
  
  var readEmAsmArgs = (sigPtr, buf) => {
      readEmAsmArgsArray.length = 0;
      var ch;
      // Most arguments are i32s, so shift the buffer pointer so it is a plain
      // index into HEAP32.
      while (ch = HEAPU8[sigPtr++]) {
        // Floats are always passed as doubles, so all types except for 'i'
        // are 8 bytes and require alignment.
        var wide = (ch != 105);
        wide &= (ch != 112);
        buf += wide && (buf % 8) ? 4 : 0;
        readEmAsmArgsArray.push(
          // Special case for pointers under wasm64 or CAN_ADDRESS_2GB mode.
          ch == 112 ? HEAPU32[((buf)>>2)] :
          ch == 106 ? HEAP64[((buf)>>3)] :
          ch == 105 ?
            HEAP32[((buf)>>2)] :
            HEAPF64[((buf)>>3)]
        );
        buf += wide ? 8 : 4;
      }
      return readEmAsmArgsArray;
    };
  var runEmAsmFunction = (code, sigPtr, argbuf) => {
      var args = readEmAsmArgs(sigPtr, argbuf);
      return ASM_CONSTS[code](...args);
    };
  var _emscripten_asm_const_int = (code, sigPtr, argbuf) => {
      return runEmAsmFunction(code, sigPtr, argbuf);
    };

  var _emscripten_cancel_main_loop = () => {
      MainLoop.pause();
      MainLoop.func = null;
    };


  var onExits = [];
  var addOnExit = (cb) => onExits.push(cb);
  var JSEvents = {
  removeAllEventListeners() {
        while (JSEvents.eventHandlers.length) {
          JSEvents._removeHandler(JSEvents.eventHandlers.length - 1);
        }
        JSEvents.deferredCalls = [];
      },
  inEventHandler:0,
  deferredCalls:[],
  deferCall(targetFunction, precedence, argsList) {
        function arraysHaveEqualContent(arrA, arrB) {
          if (arrA.length != arrB.length) return false;
  
          for (var i = 0; i < arrA.length; i++) {
            if (arrA[i] != arrB[i]) return false;
          }
          return true;
        }
        // Test if the given call was already queued, and if so, don't add it again.
        for (var call of JSEvents.deferredCalls) {
          if (call.targetFunction == targetFunction && arraysHaveEqualContent(call.argsList, argsList)) {
            return;
          }
        }
        JSEvents.deferredCalls.push({
          targetFunction,
          precedence,
          argsList
        });
  
        JSEvents.deferredCalls.sort((x,y) => x.precedence - y.precedence);
      },
  removeDeferredCalls(targetFunction) {
        JSEvents.deferredCalls = JSEvents.deferredCalls.filter((call) => call.targetFunction != targetFunction);
      },
  canPerformEventHandlerRequests() {
        // Browsers that support navigator.userActivation.isActive: https://developer.mozilla.org/en-US/docs/Web/API/UserActivation/isActive
        if (navigator.userActivation) {
          // Verify against transient activation status from UserActivation API
          // whether it is possible to perform a request here without needing to defer. See
          // https://developer.mozilla.org/en-US/docs/Web/Security/User_activation#transient_activation
          // and https://caniuse.com/mdn-api_useractivation
          return navigator.userActivation.isActive;
        }
  
        return JSEvents.inEventHandler && JSEvents.currentEventHandler.allowsDeferredCalls;
      },
  runDeferredCalls() {
        if (!JSEvents.canPerformEventHandlerRequests()) {
          return;
        }
        var deferredCalls = JSEvents.deferredCalls;
        JSEvents.deferredCalls = [];
        for (var call of deferredCalls) {
          call.targetFunction(...call.argsList);
        }
      },
  eventHandlers:[],
  removeAllHandlersOnTarget:(target, eventTypeString) => {
        for (var i = 0; i < JSEvents.eventHandlers.length; ++i) {
          if (JSEvents.eventHandlers[i].target == target &&
            (!eventTypeString || eventTypeString == JSEvents.eventHandlers[i].eventTypeString)) {
             JSEvents._removeHandler(i--);
           }
        }
      },
  _removeHandler(i) {
        var h = JSEvents.eventHandlers[i];
        h.target.removeEventListener(h.eventTypeString, h.eventListenerFunc, h.useCapture);
        JSEvents.eventHandlers.splice(i, 1);
      },
  registerOrRemoveHandler(eventHandler) {
        if (!eventHandler.target) {
          return -4;
        }
        if (eventHandler.callbackfunc) {
          eventHandler.eventListenerFunc = function(event) {
            // Increment nesting count for the event handler.
            ++JSEvents.inEventHandler;
            JSEvents.currentEventHandler = eventHandler;
            // Process any old deferred calls the user has placed.
            JSEvents.runDeferredCalls();
            // Process the actual event, calls back to user C code handler.
            eventHandler.handlerFunc(event);
            // Process any new deferred calls that were placed right now from this event handler.
            JSEvents.runDeferredCalls();
            // Out of event handler - restore nesting count.
            --JSEvents.inEventHandler;
          };
  
          eventHandler.target.addEventListener(eventHandler.eventTypeString,
                                               eventHandler.eventListenerFunc,
                                               eventHandler.useCapture);
          JSEvents.eventHandlers.push(eventHandler);
        } else {
          for (var i = 0; i < JSEvents.eventHandlers.length; ++i) {
            if (JSEvents.eventHandlers[i].target == eventHandler.target
             && JSEvents.eventHandlers[i].eventTypeString == eventHandler.eventTypeString) {
               JSEvents._removeHandler(i--);
             }
          }
        }
        return 0;
      },
  removeSingleHandler(eventHandler) {
        let success = false;
        for (let i = 0; i < JSEvents.eventHandlers.length; ++i) {
          const handler = JSEvents.eventHandlers[i];
          if (handler.target === eventHandler.target
            && handler.eventTypeId === eventHandler.eventTypeId
            && handler.callbackfunc === eventHandler.callbackfunc
            && handler.userData === eventHandler.userData) {
            // in some very rare cases (ex: Safari / fullscreen events), there is more than 1 handler (eventTypeString is different)
            JSEvents._removeHandler(i--);
            success = true;
          }
        }
        return success ? 0 : -5;
      },
  getNodeNameForTarget(target) {
        if (target == window) return '#window';
        if (target == screen) return '#screen';
        return target?.nodeName ?? '';
      },
  fullscreenEnabled() {
        return document.fullscreenEnabled
        // Safari 13.0.3 on macOS Catalina 10.15.1 still ships with prefixed webkitFullscreenEnabled.
        // TODO: If Safari at some point ships with unprefixed version, update the version check above.
        ?? document.webkitFullscreenEnabled
         ;
      },
  };
  
  
  
  
  var fillGamepadEventData = (eventStruct, e) => {
      HEAPF64[((eventStruct)>>3)] = e.timestamp;
      for (var i = 0; i < e.axes.length; ++i) {
        HEAPF64[(((eventStruct+i*8)+(16))>>3)] = e.axes[i];
      }
      for (var i = 0; i < e.buttons.length; ++i) {
        HEAP8[(eventStruct+i)+(1040)] = e.buttons[i].pressed;
        HEAPF64[(((eventStruct+i*8)+(528))>>3)] = e.buttons[i].value;
      }
      HEAP8[(eventStruct)+(1104)] = e.connected;
      HEAP32[(((eventStruct)+(1108))>>2)] = e.index;
      HEAP32[(((eventStruct)+(8))>>2)] = e.axes.length;
      HEAP32[(((eventStruct)+(12))>>2)] = e.buttons.length;
      stringToUTF8(e.id, eventStruct + 1112, 64);
      stringToUTF8(e.mapping, eventStruct + 1176, 64);
    };
  var _emscripten_get_gamepad_status = (index, gamepadState) => {
      // INVALID_PARAM is returned on a Gamepad index that never was there.
      if (index < 0 || index >= JSEvents.lastGamepadState.length) return -5;
  
      // NO_DATA is returned on a Gamepad index that was removed.
      // For previously disconnected gamepads there should be an empty slot (null/undefined/false) at the index.
      // This is because gamepads must keep their original position in the array.
      // For example, removing the first of two gamepads produces [null/undefined/false, gamepad].
      if (!JSEvents.lastGamepadState[index]) return -7;
  
      fillGamepadEventData(gamepadState, JSEvents.lastGamepadState[index]);
      return 0;
    };


  var _emscripten_get_num_gamepads = () => {
      // N.B. Do not call emscripten_get_num_gamepads() unless having first called emscripten_sample_gamepad_data(), and that has returned EMSCRIPTEN_RESULT_SUCCESS.
      // Otherwise the following line will throw an exception.
      return JSEvents.lastGamepadState.length;
    };

  var GLctx;
  
  var webgl_enable_ANGLE_instanced_arrays = (ctx) => {
      // Extension available in WebGL 1 from Firefox 26 and Google Chrome 30 onwards. Core feature in WebGL 2.
      var ext = ctx.getExtension('ANGLE_instanced_arrays');
      // Because this extension is a core function in WebGL 2, assign the extension entry points in place of
      // where the core functions will reside in WebGL 2. This way the calling code can call these without
      // having to dynamically branch depending if running against WebGL 1 or WebGL 2.
      if (ext) {
        ctx['vertexAttribDivisor'] = (index, divisor) => ext['vertexAttribDivisorANGLE'](index, divisor);
        ctx['drawArraysInstanced'] = (mode, first, count, primcount) => ext['drawArraysInstancedANGLE'](mode, first, count, primcount);
        ctx['drawElementsInstanced'] = (mode, count, type, indices, primcount) => ext['drawElementsInstancedANGLE'](mode, count, type, indices, primcount);
        return 1;
      }
    };
  
  var webgl_enable_OES_vertex_array_object = (ctx) => {
      // Extension available in WebGL 1 from Firefox 25 and WebKit 536.28/desktop Safari 6.0.3 onwards. Core feature in WebGL 2.
      var ext = ctx.getExtension('OES_vertex_array_object');
      if (ext) {
        ctx['createVertexArray'] = () => ext['createVertexArrayOES']();
        ctx['deleteVertexArray'] = (vao) => ext['deleteVertexArrayOES'](vao);
        ctx['bindVertexArray'] = (vao) => ext['bindVertexArrayOES'](vao);
        ctx['isVertexArray'] = (vao) => ext['isVertexArrayOES'](vao);
        return 1;
      }
    };
  
  var webgl_enable_WEBGL_draw_buffers = (ctx) => {
      // Extension available in WebGL 1 from Firefox 28 onwards. Core feature in WebGL 2.
      var ext = ctx.getExtension('WEBGL_draw_buffers');
      if (ext) {
        ctx['drawBuffers'] = (n, bufs) => ext['drawBuffersWEBGL'](n, bufs);
        return 1;
      }
    };
  
  var webgl_enable_WEBGL_draw_instanced_base_vertex_base_instance = (ctx) =>
      // Closure is expected to be allowed to minify the '.dibvbi' property, so not accessing it quoted.
      !!(ctx.dibvbi = ctx.getExtension('WEBGL_draw_instanced_base_vertex_base_instance'));
  
  var webgl_enable_WEBGL_multi_draw_instanced_base_vertex_base_instance = (ctx) => {
      // Closure is expected to be allowed to minify the '.mdibvbi' property, so not accessing it quoted.
      return !!(ctx.mdibvbi = ctx.getExtension('WEBGL_multi_draw_instanced_base_vertex_base_instance'));
    };
  
  var webgl_enable_EXT_polygon_offset_clamp = (ctx) =>
      !!(ctx.extPolygonOffsetClamp = ctx.getExtension('EXT_polygon_offset_clamp'));
  
  var webgl_enable_EXT_clip_control = (ctx) =>
      !!(ctx.extClipControl = ctx.getExtension('EXT_clip_control'));
  
  var webgl_enable_WEBGL_polygon_mode = (ctx) =>
      !!(ctx.webglPolygonMode = ctx.getExtension('WEBGL_polygon_mode'));
  
  var webgl_enable_WEBGL_multi_draw = (ctx) =>
      // Closure is expected to be allowed to minify the '.multiDrawWebgl' property, so not accessing it quoted.
      !!(ctx.multiDrawWebgl = ctx.getExtension('WEBGL_multi_draw'));
  
  var getEmscriptenSupportedExtensions = (ctx) => {
      // Restrict the list of advertised extensions to those that we actually
      // support.
      var supportedExtensions = [
        // WebGL 1 extensions
        'ANGLE_instanced_arrays',
        'EXT_blend_minmax',
        'EXT_disjoint_timer_query',
        'EXT_frag_depth',
        'EXT_shader_texture_lod',
        'EXT_sRGB',
        'OES_element_index_uint',
        'OES_fbo_render_mipmap',
        'OES_standard_derivatives',
        'OES_texture_float',
        'OES_texture_half_float',
        'OES_texture_half_float_linear',
        'OES_vertex_array_object',
        'WEBGL_color_buffer_float',
        'WEBGL_depth_texture',
        'WEBGL_draw_buffers',
        // WebGL 2 extensions
        'EXT_color_buffer_float',
        'EXT_conservative_depth',
        'EXT_disjoint_timer_query_webgl2',
        'EXT_texture_norm16',
        'NV_shader_noperspective_interpolation',
        'WEBGL_clip_cull_distance',
        // WebGL 1 and WebGL 2 extensions
        'EXT_clip_control',
        'EXT_color_buffer_half_float',
        'EXT_depth_clamp',
        'EXT_float_blend',
        'EXT_polygon_offset_clamp',
        'EXT_texture_compression_bptc',
        'EXT_texture_compression_rgtc',
        'EXT_texture_filter_anisotropic',
        'KHR_parallel_shader_compile',
        'OES_texture_float_linear',
        'WEBGL_blend_func_extended',
        'WEBGL_compressed_texture_astc',
        'WEBGL_compressed_texture_etc',
        'WEBGL_compressed_texture_etc1',
        'WEBGL_compressed_texture_s3tc',
        'WEBGL_compressed_texture_s3tc_srgb',
        'WEBGL_debug_renderer_info',
        'WEBGL_debug_shaders',
        'WEBGL_lose_context',
        'WEBGL_multi_draw',
        'WEBGL_polygon_mode'
      ];
      // .getSupportedExtensions() can return null if context is lost, so coerce to empty array.
      return ctx.getSupportedExtensions()?.filter(ext => supportedExtensions.includes(ext)) ?? [];
    };
  
  var registerPreMainLoop = (f) => {
      // Does nothing unless $MainLoop is included/used.
      typeof MainLoop != 'undefined' && MainLoop.preMainLoop.push(f);
    };
  
  
  var webglBufferSubData = (target, offset, size, data, src = HEAPU8) => {
      if (GL.currentContext.version >= 2) {
        size && GLctx.bufferSubData(target, offset, src, data, size);
        return;
      }
      GLctx.bufferSubData(target, offset, src.subarray(data, data + size));
    };
  
  
  
  
  var GL = {
  counter:1,
  buffers:[],
  mappedBuffers:{
  },
  programs:[],
  framebuffers:[],
  renderbuffers:[],
  textures:[],
  shaders:[],
  vaos:[],
  contexts:[],
  offscreenCanvases:{
  },
  queries:[],
  samplers:[],
  transformFeedbacks:[],
  syncs:[],
  byteSizeByTypeRoot:5120,
  byteSizeByType:[1,1,2,2,4,4,4,2,3,4,8],
  stringCache:{
  },
  stringiCache:{
  },
  unpackAlignment:4,
  unpackRowLength:0,
  recordError:(errorCode) => {
        if (!GL.lastError) {
          GL.lastError = errorCode;
        }
      },
  getNewId:(table) => {
        var ret = GL.counter++;
        for (var i = table.length; i < ret; i++) {
          table[i] = null;
        }
        // Skip over any non-null elements that might have been created by
        // glBindBuffer.
        while (table[ret]) {
          ret = GL.counter++;
        }
        return ret;
      },
  genObject:(n, buffers, createFunction, objectTable
        ) => {
        for (var i = 0; i < n; i++) {
          var buffer = GLctx[createFunction]();
          var id = buffer && GL.getNewId(objectTable);
          if (buffer) {
            buffer.name = id;
            objectTable[id] = buffer;
          } else {
            GL.recordError(0x502 /* GL_INVALID_OPERATION */);
          }
          HEAP32[(((buffers)+(i*4))>>2)] = id;
        }
      },
  MAX_TEMP_BUFFER_SIZE:2097152,
  numTempVertexBuffersPerSize:64,
  log2ceilLookup:(i) => 32 - Math.clz32(i ? i - 1 : 0),
  generateTempBuffers:(quads, context) => {
        var largestIndex = GL.log2ceilLookup(GL.MAX_TEMP_BUFFER_SIZE);
        context.tempVertexBufferCounters1 = [];
        context.tempVertexBufferCounters2 = [];
        context.tempVertexBufferCounters1.length = context.tempVertexBufferCounters2.length = largestIndex+1;
        context.tempVertexBuffers1 = [];
        context.tempVertexBuffers2 = [];
        context.tempVertexBuffers1.length = context.tempVertexBuffers2.length = largestIndex+1;
        context.tempIndexBuffers = [];
        context.tempIndexBuffers.length = largestIndex+1;
        for (var i = 0; i <= largestIndex; ++i) {
          context.tempIndexBuffers[i] = null; // Created on-demand
          context.tempVertexBufferCounters1[i] = context.tempVertexBufferCounters2[i] = 0;
          var ringbufferLength = GL.numTempVertexBuffersPerSize;
          context.tempVertexBuffers1[i] = [];
          context.tempVertexBuffers2[i] = [];
          var ringbuffer1 = context.tempVertexBuffers1[i];
          var ringbuffer2 = context.tempVertexBuffers2[i];
          ringbuffer1.length = ringbuffer2.length = ringbufferLength;
          for (var j = 0; j < ringbufferLength; ++j) {
            ringbuffer1[j] = ringbuffer2[j] = null; // Created on-demand
          }
        }
  
        if (quads) {
          // GL_QUAD indexes can be precalculated
          context.tempQuadIndexBuffer = GLctx.createBuffer();
          context.GLctx.bindBuffer(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, context.tempQuadIndexBuffer);
          var numIndexes = GL.MAX_TEMP_BUFFER_SIZE >> 1;
          var quadIndexes = new Uint16Array(numIndexes);
          var i = 0, v = 0;
          while (1) {
            quadIndexes[i++] = v;
            if (i >= numIndexes) break;
            quadIndexes[i++] = v+1;
            if (i >= numIndexes) break;
            quadIndexes[i++] = v+2;
            if (i >= numIndexes) break;
            quadIndexes[i++] = v;
            if (i >= numIndexes) break;
            quadIndexes[i++] = v+2;
            if (i >= numIndexes) break;
            quadIndexes[i++] = v+3;
            if (i >= numIndexes) break;
            v += 4;
          }
          context.GLctx.bufferData(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, quadIndexes, 0x88E4 /*GL_STATIC_DRAW*/);
          context.GLctx.bindBuffer(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, null);
        }
      },
  getTempVertexBuffer:(sizeBytes) => {
        var idx = GL.log2ceilLookup(sizeBytes);
        var ringbuffer = GL.currentContext.tempVertexBuffers1[idx];
        var nextFreeBufferIndex = GL.currentContext.tempVertexBufferCounters1[idx];
        GL.currentContext.tempVertexBufferCounters1[idx] = (GL.currentContext.tempVertexBufferCounters1[idx]+1) & (GL.numTempVertexBuffersPerSize-1);
        var vbo = ringbuffer[nextFreeBufferIndex];
        if (vbo) {
          return vbo;
        }
        var prevVBO = GLctx.getParameter(0x8894 /*GL_ARRAY_BUFFER_BINDING*/);
        ringbuffer[nextFreeBufferIndex] = GLctx.createBuffer();
        GLctx.bindBuffer(0x8892 /*GL_ARRAY_BUFFER*/, ringbuffer[nextFreeBufferIndex]);
        GLctx.bufferData(0x8892 /*GL_ARRAY_BUFFER*/, 1 << idx, 0x88E8 /*GL_DYNAMIC_DRAW*/);
        GLctx.bindBuffer(0x8892 /*GL_ARRAY_BUFFER*/, prevVBO);
        return ringbuffer[nextFreeBufferIndex];
      },
  getTempIndexBuffer:(sizeBytes) => {
        var idx = GL.log2ceilLookup(sizeBytes);
        var ibo = GL.currentContext.tempIndexBuffers[idx];
        if (ibo) {
          return ibo;
        }
        var prevIBO = GLctx.getParameter(0x8895 /*ELEMENT_ARRAY_BUFFER_BINDING*/);
        GL.currentContext.tempIndexBuffers[idx] = GLctx.createBuffer();
        GLctx.bindBuffer(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, GL.currentContext.tempIndexBuffers[idx]);
        GLctx.bufferData(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, 1 << idx, 0x88E8 /*GL_DYNAMIC_DRAW*/);
        GLctx.bindBuffer(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, prevIBO);
        return GL.currentContext.tempIndexBuffers[idx];
      },
  newRenderingFrameStarted:() => {
        if (!GL.currentContext) {
          return;
        }
        var vb = GL.currentContext.tempVertexBuffers1;
        GL.currentContext.tempVertexBuffers1 = GL.currentContext.tempVertexBuffers2;
        GL.currentContext.tempVertexBuffers2 = vb;
        vb = GL.currentContext.tempVertexBufferCounters1;
        GL.currentContext.tempVertexBufferCounters1 = GL.currentContext.tempVertexBufferCounters2;
        GL.currentContext.tempVertexBufferCounters2 = vb;
        var largestIndex = GL.log2ceilLookup(GL.MAX_TEMP_BUFFER_SIZE);
        for (var i = 0; i <= largestIndex; ++i) {
          GL.currentContext.tempVertexBufferCounters1[i] = 0;
        }
      },
  getSource:(shader, count, string, length) => {
        var source = '';
        for (var i = 0; i < count; ++i) {
          var len = length ? HEAPU32[(((length)+(i*4))>>2)] : undefined;
          source += UTF8ToString(HEAPU32[(((string)+(i*4))>>2)], len);
        }
        return source;
      },
  calcBufLength:(size, type, stride, count) => {
        if (stride > 0) {
          return count * stride;  // XXXvlad this is not exactly correct I don't think
        }
        var typeSize = GL.byteSizeByType[type - GL.byteSizeByTypeRoot];
        return size * typeSize * count;
      },
  usedTempBuffers:[],
  preDrawHandleClientVertexAttribBindings:(count) => {
        GL.resetBufferBinding = false;
  
        // TODO: initial pass to detect ranges we need to upload, might not need
        // an upload per attrib
        for (var i = 0; i < GL.currentContext.maxVertexAttribs; ++i) {
          var cb = GL.currentContext.clientBuffers[i];
          if (!cb.clientside || !cb.enabled) continue;
  
          GL.resetBufferBinding = true;
  
          var size = GL.calcBufLength(cb.size, cb.type, cb.stride, count);
          var buf = GL.getTempVertexBuffer(size);
          GLctx.bindBuffer(0x8892 /*GL_ARRAY_BUFFER*/, buf);
          webglBufferSubData(0x8892 /*GL_ARRAY_BUFFER*/, 0, size, cb.ptr);
          cb.vertexAttribPointerAdaptor.call(GLctx, i, cb.size, cb.type, cb.normalized, cb.stride, 0);
        }
      },
  postDrawHandleClientVertexAttribBindings:() => {
        if (GL.resetBufferBinding) {
          GLctx.bindBuffer(0x8892 /*GL_ARRAY_BUFFER*/, GL.buffers[GLctx.currentArrayBufferBinding]);
        }
      },
  createContext:(/** @type {HTMLCanvasElement} */ canvas, webGLContextAttributes) => {
  
        // BUG: Workaround Safari WebGL issue: After successfully acquiring WebGL
        // context on a canvas, calling .getContext() will always return that
        // context independent of which 'webgl' or 'webgl2'
        // context version was passed. See:
        //   https://webkit.org/b/222758
        // and:
        //   https://github.com/emscripten-core/emscripten/issues/13295.
        // TODO: Once the bug is fixed and shipped in Safari, adjust the Safari
        // version field in above check.
        if (!canvas.getContextSafariWebGL2Fixed) {
          canvas.getContextSafariWebGL2Fixed = canvas.getContext;
          /** @type {function(this:HTMLCanvasElement, string, (Object|null)=): (Object|null)} */
          function fixedGetContext(ver, attrs) {
            var gl = canvas.getContextSafariWebGL2Fixed(ver, attrs);
            return ((ver == 'webgl') == (gl instanceof WebGLRenderingContext)) ? gl : null;
          }
          canvas.getContext = fixedGetContext;
        }
  
        var ctx =
          (webGLContextAttributes.majorVersion > 1)
          ? canvas.getContext('webgl2', webGLContextAttributes) :
          canvas.getContext('webgl', webGLContextAttributes);
  
        if (!ctx) return 0;
  
        var handle = GL.registerContext(ctx, webGLContextAttributes);
  
        return handle;
      },
  registerContext:(ctx, webGLContextAttributes) => {
        // without pthreads a context is just an integer ID
        var handle = GL.getNewId(GL.contexts);
  
        var context = {
          handle,
          attributes: webGLContextAttributes,
          version: webGLContextAttributes.majorVersion,
          GLctx: ctx
        };
  
        // Store the created context object so that we can access the context
        // given a canvas without having to pass the parameters again.
        if (ctx.canvas) ctx.canvas.GLctxObject = context;
        GL.contexts[handle] = context;
        if (typeof webGLContextAttributes.enableExtensionsByDefault == 'undefined' || webGLContextAttributes.enableExtensionsByDefault) {
          GL.initExtensions(context);
        }
  
        context.maxVertexAttribs = context.GLctx.getParameter(0x8869 /*GL_MAX_VERTEX_ATTRIBS*/);
        context.clientBuffers = [];
        for (var i = 0; i < context.maxVertexAttribs; i++) {
          context.clientBuffers[i] = {
            enabled: false,
            clientside: false,
            size: 0,
            type: 0,
            normalized: 0,
            stride: 0,
            ptr: 0,
            vertexAttribPointerAdaptor: null,
          };
        }
  
        GL.generateTempBuffers(false, context);
  
        return handle;
      },
  makeContextCurrent:(contextHandle) => {
  
        // Active Emscripten GL layer context object.
        GL.currentContext = GL.contexts[contextHandle];
        // Active WebGL context object.
        Module['ctx'] = GLctx = GL.currentContext?.GLctx;
        return !(contextHandle && !GLctx);
      },
  getContext:(contextHandle) => {
        return GL.contexts[contextHandle];
      },
  deleteContext:(contextHandle) => {
        if (GL.currentContext === GL.contexts[contextHandle]) {
          GL.currentContext = null;
        }
        if (typeof JSEvents == 'object') {
          // Release all JS event handlers on the DOM element that the GL context is
          // associated with since the context is now deleted.
          JSEvents.removeAllHandlersOnTarget(GL.contexts[contextHandle].GLctx.canvas);
        }
        // Make sure the canvas object no longer refers to the context object so
        // there are no GC surprises.
        if (GL.contexts[contextHandle]?.GLctx.canvas) {
          GL.contexts[contextHandle].GLctx.canvas.GLctxObject = undefined;
        }
        GL.contexts[contextHandle] = null;
      },
  initExtensions:(context) => {
        // If this function is called without a specific context object, init the
        // extensions of the currently active context.
        context ||= GL.currentContext;
  
        if (context.initExtensionsDone) return;
        context.initExtensionsDone = true;
  
        var GLctx = context.GLctx;
  
        // Detect the presence of a few extensions manually, since the GL interop
        // layer itself will need to know if they exist.
  
        // Extensions that are available in both WebGL 1 and WebGL 2
        webgl_enable_WEBGL_multi_draw(GLctx);
        webgl_enable_EXT_polygon_offset_clamp(GLctx);
        webgl_enable_EXT_clip_control(GLctx);
        webgl_enable_WEBGL_polygon_mode(GLctx);
        // Extensions that are only available in WebGL 1 (the calls will be no-ops
        // if called on a WebGL 2 context active)
        webgl_enable_ANGLE_instanced_arrays(GLctx);
        webgl_enable_OES_vertex_array_object(GLctx);
        webgl_enable_WEBGL_draw_buffers(GLctx);
        // Extensions that are available from WebGL >= 2 (no-op if called on a WebGL 1 context active)
        webgl_enable_WEBGL_draw_instanced_base_vertex_base_instance(GLctx);
        webgl_enable_WEBGL_multi_draw_instanced_base_vertex_base_instance(GLctx);
  
        // On WebGL 2, EXT_disjoint_timer_query is replaced with an alternative
        // that's based on core APIs, and exposes only the queryCounterEXT()
        // entrypoint.
        if (context.version >= 2) {
          GLctx.disjointTimerQueryExt = GLctx.getExtension('EXT_disjoint_timer_query_webgl2');
        }
  
        // However, Firefox exposes the WebGL 1 version on WebGL 2 as well and
        // thus we look for the WebGL 1 version again if the WebGL 2 version
        // isn't present. https://bugzil.la/1328882
        if (context.version < 2 || !GLctx.disjointTimerQueryExt)
        {
          GLctx.disjointTimerQueryExt = GLctx.getExtension('EXT_disjoint_timer_query');
        }
  
        for (var ext of getEmscriptenSupportedExtensions(GLctx)) {
          // WEBGL_lose_context, WEBGL_debug_renderer_info and WEBGL_debug_shaders
          // are not enabled by default.
          if (!ext.includes('lose_context') && !ext.includes('debug')) {
            // Call .getExtension() to enable that extension permanently.
            GLctx.getExtension(ext);
          }
        }
      },
  };
  var _emscripten_glActiveTexture = (x0) => GLctx.activeTexture(x0);

  var _emscripten_glAttachShader = (program, shader) => {
      GLctx.attachShader(GL.programs[program], GL.shaders[shader]);
    };

  var _emscripten_glBeginQuery = (target, id) => {
      GLctx.beginQuery(target, GL.queries[id]);
    };

  var _emscripten_glBeginQueryEXT = (target, id) => {
      GLctx.disjointTimerQueryExt['beginQueryEXT'](target, GL.queries[id]);
    };

  var _emscripten_glBeginTransformFeedback = (x0) => GLctx.beginTransformFeedback(x0);

  
  var _emscripten_glBindAttribLocation = (program, index, name) => {
      GLctx.bindAttribLocation(GL.programs[program], index, UTF8ToString(name));
    };

  var _emscripten_glBindBuffer = (target, buffer) => {
      // Calling glBindBuffer with an unknown buffer will implicitly create a
      // new one.  Here we bypass `GL.counter` and directly using the ID passed
      // in.
      if (buffer && !GL.buffers[buffer]) {
        var b = GLctx.createBuffer();
        b.name = buffer;
        GL.buffers[buffer] = b;
      }
      if (target == 0x8892 /*GL_ARRAY_BUFFER*/) {
        GLctx.currentArrayBufferBinding = buffer;
      } else if (target == 0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/) {
        GLctx.currentElementArrayBufferBinding = buffer;
      }
  
      if (target == 0x88EB /*GL_PIXEL_PACK_BUFFER*/) {
        // In WebGL 2 glReadPixels entry point, we need to use a different WebGL 2
        // API function call when a buffer is bound to
        // GL_PIXEL_PACK_BUFFER_BINDING point, so must keep track whether that
        // binding point is non-null to know what is the proper API function to
        // call.
        GLctx.currentPixelPackBufferBinding = buffer;
      } else if (target == 0x88EC /*GL_PIXEL_UNPACK_BUFFER*/) {
        // In WebGL 2 gl(Compressed)Tex(Sub)Image[23]D entry points, we need to
        // use a different WebGL 2 API function call when a buffer is bound to
        // GL_PIXEL_UNPACK_BUFFER_BINDING point, so must keep track whether that
        // binding point is non-null to know what is the proper API function to
        // call.
        GLctx.currentPixelUnpackBufferBinding = buffer;
      }
      GLctx.bindBuffer(target, GL.buffers[buffer]);
    };

  var _emscripten_glBindBufferBase = (target, index, buffer) => {
      GLctx.bindBufferBase(target, index, GL.buffers[buffer]);
    };

  var _emscripten_glBindBufferRange = (target, index, buffer, offset, ptrsize) => {
      GLctx.bindBufferRange(target, index, GL.buffers[buffer], offset, ptrsize);
    };

  var _emscripten_glBindFramebuffer = (target, framebuffer) => {
  
      GLctx.bindFramebuffer(target, GL.framebuffers[framebuffer]);
  
    };

  var _emscripten_glBindRenderbuffer = (target, renderbuffer) => {
      GLctx.bindRenderbuffer(target, GL.renderbuffers[renderbuffer]);
    };

  var _emscripten_glBindSampler = (unit, sampler) => {
      GLctx.bindSampler(unit, GL.samplers[sampler]);
    };

  var _emscripten_glBindTexture = (target, texture) => {
      GLctx.bindTexture(target, GL.textures[texture]);
    };

  var _emscripten_glBindTransformFeedback = (target, id) => {
      GLctx.bindTransformFeedback(target, GL.transformFeedbacks[id]);
    };

  var _emscripten_glBindVertexArray = (vao) => {
      GLctx.bindVertexArray(GL.vaos[vao]);
      var ibo = GLctx.getParameter(0x8895 /*ELEMENT_ARRAY_BUFFER_BINDING*/);
      GLctx.currentElementArrayBufferBinding = ibo ? (ibo.name | 0) : 0;
    };

  
  var _glBindVertexArray = _emscripten_glBindVertexArray;
  var _emscripten_glBindVertexArrayOES = _glBindVertexArray;

  var _emscripten_glBlendColor = (x0, x1, x2, x3) => GLctx.blendColor(x0, x1, x2, x3);

  var _emscripten_glBlendEquation = (x0) => GLctx.blendEquation(x0);

  var _emscripten_glBlendEquationSeparate = (x0, x1) => GLctx.blendEquationSeparate(x0, x1);

  var _emscripten_glBlendFunc = (x0, x1) => GLctx.blendFunc(x0, x1);

  var _emscripten_glBlendFuncSeparate = (x0, x1, x2, x3) => GLctx.blendFuncSeparate(x0, x1, x2, x3);

  var _emscripten_glBlitFramebuffer = (x0, x1, x2, x3, x4, x5, x6, x7, x8, x9) => GLctx.blitFramebuffer(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9);

  
  var _emscripten_glBufferData = (target, size, data, usage) => {
  
      if (GL.currentContext.version >= 2) {
        // If size is zero, WebGL would interpret uploading the whole input
        // arraybuffer (starting from given offset), which would not make sense in
        // WebAssembly, so avoid uploading if size is zero. However we must still
        // call bufferData to establish a backing storage of zero bytes.
        if (data && size) {
          GLctx.bufferData(target, HEAPU8, usage, data, size);
        } else {
          GLctx.bufferData(target, size, usage);
        }
        return;
      }
      // N.b. here first form specifies a heap subarray, second form an integer
      // size, so the ?: code here is polymorphic. It is advised to avoid
      // randomly mixing both uses in calling code, to avoid any potential JS
      // engine JIT issues.
      GLctx.bufferData(target, data ? HEAPU8.subarray(data, data+size) : size, usage);
    };

  
  var _emscripten_glBufferSubData = (target, offset, size, data) => webglBufferSubData(target, offset, size, data);

  var _emscripten_glCheckFramebufferStatus = (x0) => GLctx.checkFramebufferStatus(x0);

  var _emscripten_glClear = (x0) => GLctx.clear(x0);

  var _emscripten_glClearBufferfi = (x0, x1, x2, x3) => GLctx.clearBufferfi(x0, x1, x2, x3);

  var _emscripten_glClearBufferfv = (buffer, drawbuffer, value) => {
  
      GLctx.clearBufferfv(buffer, drawbuffer, HEAPF32, ((value)>>2));
    };

  var _emscripten_glClearBufferiv = (buffer, drawbuffer, value) => {
  
      GLctx.clearBufferiv(buffer, drawbuffer, HEAP32, ((value)>>2));
    };

  var _emscripten_glClearBufferuiv = (buffer, drawbuffer, value) => {
  
      GLctx.clearBufferuiv(buffer, drawbuffer, HEAPU32, ((value)>>2));
    };

  var _emscripten_glClearColor = (x0, x1, x2, x3) => GLctx.clearColor(x0, x1, x2, x3);

  var _emscripten_glClearDepthf = (x0) => GLctx.clearDepth(x0);

  var _emscripten_glClearStencil = (x0) => GLctx.clearStencil(x0);

  var _emscripten_glClientWaitSync = (sync, flags, timeout) => {
      // WebGL2 vs GLES3 differences: in GLES3, the timeout parameter is a uint64, where 0xFFFFFFFFFFFFFFFFULL means GL_TIMEOUT_IGNORED.
      // In JS, there's no 64-bit value types, so instead timeout is taken to be signed, and GL_TIMEOUT_IGNORED is given value -1.
      // Inherently the value accepted in the timeout is lossy, and can't take in arbitrary u64 bit pattern (but most likely doesn't matter)
      // See https://www.khronos.org/registry/webgl/specs/latest/2.0/#5.15
      timeout = Number(timeout);
      return GLctx.clientWaitSync(GL.syncs[sync], flags, timeout);
    };

  var _emscripten_glClipControlEXT = (origin, depth) => {
      GLctx.extClipControl['clipControlEXT'](origin, depth);
    };

  var _emscripten_glColorMask = (red, green, blue, alpha) => {
      GLctx.colorMask(!!red, !!green, !!blue, !!alpha);
    };

  var _emscripten_glCompileShader = (shader) => {
      GLctx.compileShader(GL.shaders[shader]);
    };

  
  var _emscripten_glCompressedTexImage2D = (target, level, internalFormat, width, height, border, imageSize, data) => {
      // `data` may be null here, which means "allocate uninitialized space but
      // don't upload" in GLES parlance, but `compressedTexImage2D` requires the
      // final data parameter, so we simply pass a heap view starting at zero
      // effectively uploading whatever happens to be near address zero.  See
      // https://github.com/emscripten-core/emscripten/issues/19300.
      if (GL.currentContext.version >= 2) {
        if (GLctx.currentPixelUnpackBufferBinding || !imageSize) {
          GLctx.compressedTexImage2D(target, level, internalFormat, width, height, border, imageSize, data);
          return;
        }
        GLctx.compressedTexImage2D(target, level, internalFormat, width, height, border, HEAPU8, data, imageSize);
        return;
      }
      GLctx.compressedTexImage2D(target, level, internalFormat, width, height, border, HEAPU8.subarray(data, data + imageSize));
    };

  var _emscripten_glCompressedTexImage3D = (target, level, internalFormat, width, height, depth, border, imageSize, data) => {
      if (GLctx.currentPixelUnpackBufferBinding) {
        GLctx.compressedTexImage3D(target, level, internalFormat, width, height, depth, border, imageSize, data);
      } else {
        GLctx.compressedTexImage3D(target, level, internalFormat, width, height, depth, border, HEAPU8, data, imageSize);
      }
    };

  
  var _emscripten_glCompressedTexSubImage2D = (target, level, xoffset, yoffset, width, height, format, imageSize, data) => {
      if (GL.currentContext.version >= 2) {
        if (GLctx.currentPixelUnpackBufferBinding || !imageSize) {
          GLctx.compressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
          return;
        }
        GLctx.compressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, HEAPU8, data, imageSize);
        return;
      }
      GLctx.compressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, HEAPU8.subarray(data, data + imageSize));
    };

  var _emscripten_glCompressedTexSubImage3D = (target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data) => {
      if (GLctx.currentPixelUnpackBufferBinding) {
        GLctx.compressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
      } else {
        GLctx.compressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, HEAPU8, data, imageSize);
      }
    };

  var _emscripten_glCopyBufferSubData = (x0, x1, x2, x3, x4) => GLctx.copyBufferSubData(x0, x1, x2, x3, x4);

  var _emscripten_glCopyTexImage2D = (x0, x1, x2, x3, x4, x5, x6, x7) => GLctx.copyTexImage2D(x0, x1, x2, x3, x4, x5, x6, x7);

  var _emscripten_glCopyTexSubImage2D = (x0, x1, x2, x3, x4, x5, x6, x7) => GLctx.copyTexSubImage2D(x0, x1, x2, x3, x4, x5, x6, x7);

  var _emscripten_glCopyTexSubImage3D = (x0, x1, x2, x3, x4, x5, x6, x7, x8) => GLctx.copyTexSubImage3D(x0, x1, x2, x3, x4, x5, x6, x7, x8);

  var _emscripten_glCreateProgram = () => {
      var id = GL.getNewId(GL.programs);
      var program = GLctx.createProgram();
      // Store additional information needed for each shader program:
      program.name = id;
      // Lazy cache results of
      // glGetProgramiv(GL_ACTIVE_UNIFORM_MAX_LENGTH/GL_ACTIVE_ATTRIBUTE_MAX_LENGTH/GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH)
      program.maxUniformLength = program.maxAttributeLength = program.maxUniformBlockNameLength = 0;
      program.uniformIdCounter = 1;
      GL.programs[id] = program;
      return id;
    };

  var _emscripten_glCreateShader = (shaderType) => {
      var id = GL.getNewId(GL.shaders);
      GL.shaders[id] = GLctx.createShader(shaderType);
  
      return id;
    };

  var _emscripten_glCullFace = (x0) => GLctx.cullFace(x0);

  
  var _emscripten_glDeleteBuffers = (n, buffers) => {
      for (var i = 0; i < n; i++) {
        var id = HEAP32[(((buffers)+(i*4))>>2)];
        var buffer = GL.buffers[id];
  
        // From spec: "glDeleteBuffers silently ignores 0's and names that do not
        // correspond to existing buffer objects."
        if (!buffer) continue;
  
        GLctx.deleteBuffer(buffer);
        buffer.name = 0;
        GL.buffers[id] = null;
  
        if (id == GLctx.currentArrayBufferBinding) GLctx.currentArrayBufferBinding = 0;
        if (id == GLctx.currentElementArrayBufferBinding) GLctx.currentElementArrayBufferBinding = 0;
        if (id == GLctx.currentPixelPackBufferBinding) GLctx.currentPixelPackBufferBinding = 0;
        if (id == GLctx.currentPixelUnpackBufferBinding) GLctx.currentPixelUnpackBufferBinding = 0;
      }
    };

  
  var _emscripten_glDeleteFramebuffers = (n, framebuffers) => {
      for (var i = 0; i < n; ++i) {
        var id = HEAP32[(((framebuffers)+(i*4))>>2)];
        var framebuffer = GL.framebuffers[id];
        if (!framebuffer) continue; // GL spec: "glDeleteFramebuffers silently ignores 0s and names that do not correspond to existing framebuffer objects".
        GLctx.deleteFramebuffer(framebuffer);
        framebuffer.name = 0;
        GL.framebuffers[id] = null;
      }
    };

  var _emscripten_glDeleteProgram = (id) => {
      if (!id) return;
      var program = GL.programs[id];
      if (!program) {
        // glDeleteProgram actually signals an error when deleting a nonexisting
        // object, unlike some other GL delete functions.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      GLctx.deleteProgram(program);
      program.name = 0;
      GL.programs[id] = null;
    };

  var _emscripten_glDeleteQueries = (n, ids) => {
      for (var i = 0; i < n; i++) {
        var id = HEAP32[(((ids)+(i*4))>>2)];
        var query = GL.queries[id];
        if (!query) continue; // GL spec: "unused names in ids are ignored, as is the name zero."
        GLctx.deleteQuery(query);
        GL.queries[id] = null;
      }
    };

  
  var _emscripten_glDeleteQueriesEXT = (n, ids) => {
      for (var i = 0; i < n; i++) {
        var id = HEAP32[(((ids)+(i*4))>>2)];
        var query = GL.queries[id];
        if (!query) continue; // GL spec: "unused names in ids are ignored, as is the name zero."
        GLctx.disjointTimerQueryExt['deleteQueryEXT'](query);
        GL.queries[id] = null;
      }
    };

  
  var _emscripten_glDeleteRenderbuffers = (n, renderbuffers) => {
      for (var i = 0; i < n; i++) {
        var id = HEAP32[(((renderbuffers)+(i*4))>>2)];
        var renderbuffer = GL.renderbuffers[id];
        if (!renderbuffer) continue; // GL spec: "glDeleteRenderbuffers silently ignores 0s and names that do not correspond to existing renderbuffer objects".
        GLctx.deleteRenderbuffer(renderbuffer);
        renderbuffer.name = 0;
        GL.renderbuffers[id] = null;
      }
    };

  var _emscripten_glDeleteSamplers = (n, samplers) => {
      for (var i = 0; i < n; i++) {
        var id = HEAP32[(((samplers)+(i*4))>>2)];
        var sampler = GL.samplers[id];
        if (!sampler) continue;
        GLctx.deleteSampler(sampler);
        sampler.name = 0;
        GL.samplers[id] = null;
      }
    };

  var _emscripten_glDeleteShader = (id) => {
      if (!id) return;
      var shader = GL.shaders[id];
      if (!shader) {
        // glDeleteShader actually signals an error when deleting a nonexisting
        // object, unlike some other GL delete functions.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      GLctx.deleteShader(shader);
      GL.shaders[id] = null;
    };

  var _emscripten_glDeleteSync = (id) => {
      if (!id) return;
      var sync = GL.syncs[id];
      if (!sync) { // glDeleteSync signals an error when deleting a nonexisting object, unlike some other GL delete functions.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      GLctx.deleteSync(sync);
      sync.name = 0;
      GL.syncs[id] = null;
    };

  
  var _emscripten_glDeleteTextures = (n, textures) => {
      for (var i = 0; i < n; i++) {
        var id = HEAP32[(((textures)+(i*4))>>2)];
        var texture = GL.textures[id];
        // GL spec: "glDeleteTextures silently ignores 0s and names that do not
        // correspond to existing textures".
        if (!texture) continue;
        GLctx.deleteTexture(texture);
        texture.name = 0;
        GL.textures[id] = null;
      }
    };

  var _emscripten_glDeleteTransformFeedbacks = (n, ids) => {
      for (var i = 0; i < n; i++) {
        var id = HEAP32[(((ids)+(i*4))>>2)];
        var transformFeedback = GL.transformFeedbacks[id];
        if (!transformFeedback) continue; // GL spec: "unused names in ids are ignored, as is the name zero."
        GLctx.deleteTransformFeedback(transformFeedback);
        transformFeedback.name = 0;
        GL.transformFeedbacks[id] = null;
      }
    };

  
  var _emscripten_glDeleteVertexArrays = (n, vaos) => {
      for (var i = 0; i < n; i++) {
        var id = HEAP32[(((vaos)+(i*4))>>2)];
        GLctx.deleteVertexArray(GL.vaos[id]);
        GL.vaos[id] = null;
      }
    };

  
  var _glDeleteVertexArrays = _emscripten_glDeleteVertexArrays;
  var _emscripten_glDeleteVertexArraysOES = _glDeleteVertexArrays;

  var _emscripten_glDepthFunc = (x0) => GLctx.depthFunc(x0);

  var _emscripten_glDepthMask = (flag) => {
      GLctx.depthMask(!!flag);
    };

  var _emscripten_glDepthRangef = (x0, x1) => GLctx.depthRange(x0, x1);

  var _emscripten_glDetachShader = (program, shader) => {
      GLctx.detachShader(GL.programs[program], GL.shaders[shader]);
    };

  var _emscripten_glDisable = (x0) => GLctx.disable(x0);

  var _emscripten_glDisableVertexAttribArray = (index) => {
      var cb = GL.currentContext.clientBuffers[index];
      cb.enabled = false;
      GLctx.disableVertexAttribArray(index);
    };

  var _emscripten_glDrawArrays = (mode, first, count) => {
      // bind any client-side buffers
      GL.preDrawHandleClientVertexAttribBindings(first + count);
  
      GLctx.drawArrays(mode, first, count);
  
      GL.postDrawHandleClientVertexAttribBindings();
    };

  var _emscripten_glDrawArraysInstanced = (mode, first, count, primcount) => {
      GLctx.drawArraysInstanced(mode, first, count, primcount);
    };

  
  var _glDrawArraysInstanced = _emscripten_glDrawArraysInstanced;
  var _emscripten_glDrawArraysInstancedANGLE = _glDrawArraysInstanced;

  
  var _emscripten_glDrawArraysInstancedARB = _glDrawArraysInstanced;

  
  var _emscripten_glDrawArraysInstancedEXT = _glDrawArraysInstanced;

  
  var _emscripten_glDrawArraysInstancedNV = _glDrawArraysInstanced;

  var tempFixedLengthArray = [];
  
  
  var _emscripten_glDrawBuffers = (n, bufs) => {
  
      var bufArray = tempFixedLengthArray[n];
      for (var i = 0; i < n; i++) {
        bufArray[i] = HEAP32[(((bufs)+(i*4))>>2)];
      }
  
      GLctx.drawBuffers(bufArray);
    };

  
  var _glDrawBuffers = _emscripten_glDrawBuffers;
  var _emscripten_glDrawBuffersEXT = _glDrawBuffers;

  
  var _emscripten_glDrawBuffersWEBGL = _glDrawBuffers;

  
  
  var _emscripten_glDrawElements = (mode, count, type, indices) => {
      var buf;
      var vertexes = 0;
      if (!GLctx.currentElementArrayBufferBinding) {
        var size = GL.calcBufLength(1, type, 0, count);
        buf = GL.getTempIndexBuffer(size);
        GLctx.bindBuffer(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, buf);
        webglBufferSubData(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, 0, size, indices);
  
        // Calculating vertex count if shader's attribute data is on client side
        if (count > 0) {
          for (var i = 0; i < GL.currentContext.maxVertexAttribs; ++i) {
            var cb = GL.currentContext.clientBuffers[i];
            if (cb.clientside && cb.enabled) {
              let arrayClass;
              switch(type) {
                case 0x1401 /* GL_UNSIGNED_BYTE */: arrayClass = Uint8Array; break;
                case 0x1403 /* GL_UNSIGNED_SHORT */: arrayClass = Uint16Array; break;
                case 0x1405 /* GL_UNSIGNED_INT */: arrayClass = Uint32Array; break;
                default:
                  GL.recordError(0x502 /* GL_INVALID_OPERATION */);
                  return;
              }
  
              vertexes = new arrayClass(HEAPU8.buffer, indices, count).reduce((max, current) => Math.max(max, current)) + 1;
              break;
            }
          }
        }
  
        // the index is now 0
        indices = 0;
      }
  
      // bind any client-side buffers
      GL.preDrawHandleClientVertexAttribBindings(vertexes);
  
      GLctx.drawElements(mode, count, type, indices);
  
      GL.postDrawHandleClientVertexAttribBindings(count);
  
      if (!GLctx.currentElementArrayBufferBinding) {
        GLctx.bindBuffer(0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/, null);
      }
    };

  var _emscripten_glDrawElementsInstanced = (mode, count, type, indices, primcount) => {
      GLctx.drawElementsInstanced(mode, count, type, indices, primcount);
    };

  
  var _glDrawElementsInstanced = _emscripten_glDrawElementsInstanced;
  var _emscripten_glDrawElementsInstancedANGLE = _glDrawElementsInstanced;

  
  var _emscripten_glDrawElementsInstancedARB = _glDrawElementsInstanced;

  
  var _emscripten_glDrawElementsInstancedEXT = _glDrawElementsInstanced;

  
  var _emscripten_glDrawElementsInstancedNV = _glDrawElementsInstanced;

  var _glDrawElements = _emscripten_glDrawElements;
  var _emscripten_glDrawRangeElements = (mode, start, end, count, type, indices) => {
      // TODO: This should be a trivial pass-through function registered at the bottom of this page as
      // glFuncs[6][1] += ' drawRangeElements';
      // but due to https://bugzil.la/1202427,
      // we work around by ignoring the range.
      _glDrawElements(mode, count, type, indices);
    };

  var _emscripten_glEnable = (x0) => GLctx.enable(x0);

  var _emscripten_glEnableVertexAttribArray = (index) => {
      var cb = GL.currentContext.clientBuffers[index];
      cb.enabled = true;
      GLctx.enableVertexAttribArray(index);
    };

  var _emscripten_glEndQuery = (x0) => GLctx.endQuery(x0);

  var _emscripten_glEndQueryEXT = (target) => {
      GLctx.disjointTimerQueryExt['endQueryEXT'](target);
    };

  var _emscripten_glEndTransformFeedback = () => GLctx.endTransformFeedback();

  var _emscripten_glFenceSync = (condition, flags) => {
      var sync = GLctx.fenceSync(condition, flags);
      if (sync) {
        var id = GL.getNewId(GL.syncs);
        sync.name = id;
        GL.syncs[id] = sync;
        return id;
      }
      return 0; // Failed to create a sync object
    };

  var _emscripten_glFinish = () => GLctx.finish();

  var _emscripten_glFlush = () => GLctx.flush();

  var emscriptenWebGLGetBufferBinding = (target) => {
      switch (target) {
        case 0x8892 /*GL_ARRAY_BUFFER*/: target = 0x8894 /*GL_ARRAY_BUFFER_BINDING*/; break;
        case 0x8893 /*GL_ELEMENT_ARRAY_BUFFER*/: target = 0x8895 /*GL_ELEMENT_ARRAY_BUFFER_BINDING*/; break;
        case 0x88EB /*GL_PIXEL_PACK_BUFFER*/: target = 0x88ED /*GL_PIXEL_PACK_BUFFER_BINDING*/; break;
        case 0x88EC /*GL_PIXEL_UNPACK_BUFFER*/: target = 0x88EF /*GL_PIXEL_UNPACK_BUFFER_BINDING*/; break;
        case 0x8C8E /*GL_TRANSFORM_FEEDBACK_BUFFER*/: target = 0x8C8F /*GL_TRANSFORM_FEEDBACK_BUFFER_BINDING*/; break;
        case 0x8F36 /*GL_COPY_READ_BUFFER*/: target = 0x8F36 /*GL_COPY_READ_BUFFER_BINDING*/; break;
        case 0x8F37 /*GL_COPY_WRITE_BUFFER*/: target = 0x8F37 /*GL_COPY_WRITE_BUFFER_BINDING*/; break;
        case 0x8A11 /*GL_UNIFORM_BUFFER*/: target = 0x8A28 /*GL_UNIFORM_BUFFER_BINDING*/; break;
        // In default case, fall through and assume passed one of the _BINDING enums directly.
      }
      var buffer = GLctx.getParameter(target);
      if (buffer) return buffer.name|0;
      else return 0;
    };
  
  var emscriptenWebGLValidateMapBufferTarget = (target) => {
      switch (target) {
        case 0x8892: // GL_ARRAY_BUFFER
        case 0x8893: // GL_ELEMENT_ARRAY_BUFFER
        case 0x8F36: // GL_COPY_READ_BUFFER
        case 0x8F37: // GL_COPY_WRITE_BUFFER
        case 0x88EB: // GL_PIXEL_PACK_BUFFER
        case 0x88EC: // GL_PIXEL_UNPACK_BUFFER
        case 0x8C2A: // GL_TEXTURE_BUFFER
        case 0x8C8E: // GL_TRANSFORM_FEEDBACK_BUFFER
        case 0x8A11: // GL_UNIFORM_BUFFER
          return true;
        default:
          return false;
      }
    };
  
  
  var _emscripten_glFlushMappedBufferRange = (target, offset, length) => {
      if (!emscriptenWebGLValidateMapBufferTarget(target)) {
        GL.recordError(0x500/*GL_INVALID_ENUM*/);
        err('GL_INVALID_ENUM in glFlushMappedBufferRange');
        return;
      }
  
      var mapping = GL.mappedBuffers[emscriptenWebGLGetBufferBinding(target)];
      if (!mapping) {
        GL.recordError(0x502 /* GL_INVALID_OPERATION */);
        err('buffer was never mapped in glFlushMappedBufferRange');
        return;
      }
  
      if (!(mapping.access & 0x10)) {
        GL.recordError(0x502 /* GL_INVALID_OPERATION */);
        err('buffer was not mapped with GL_MAP_FLUSH_EXPLICIT_BIT in glFlushMappedBufferRange');
        return;
      }
      if (offset < 0 || length < 0 || offset + length > mapping.length) {
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        err('invalid range in glFlushMappedBufferRange');
        return;
      }
  
      webglBufferSubData(target, mapping.offset, length, mapping.mem + offset);
    };

  var _emscripten_glFramebufferRenderbuffer = (target, attachment, renderbuffertarget, renderbuffer) => {
      GLctx.framebufferRenderbuffer(target, attachment, renderbuffertarget,
                                         GL.renderbuffers[renderbuffer]);
    };

  var _emscripten_glFramebufferTexture2D = (target, attachment, textarget, texture, level) => {
      GLctx.framebufferTexture2D(target, attachment, textarget,
                                      GL.textures[texture], level);
    };

  var _emscripten_glFramebufferTextureLayer = (target, attachment, texture, level, layer) => {
      GLctx.framebufferTextureLayer(target, attachment, GL.textures[texture], level, layer);
    };

  var _emscripten_glFrontFace = (x0) => GLctx.frontFace(x0);

  var _emscripten_glGenBuffers = (n, buffers) => {
      GL.genObject(n, buffers, 'createBuffer', GL.buffers
        );
    };

  var _emscripten_glGenFramebuffers = (n, ids) => {
      GL.genObject(n, ids, 'createFramebuffer', GL.framebuffers
        );
    };

  var _emscripten_glGenQueries = (n, ids) => {
      GL.genObject(n, ids, 'createQuery', GL.queries
        );
    };

  
  var _emscripten_glGenQueriesEXT = (n, ids) => {
      for (var i = 0; i < n; i++) {
        var query = GLctx.disjointTimerQueryExt['createQueryEXT']();
        if (!query) {
          GL.recordError(0x502 /* GL_INVALID_OPERATION */);
          while (i < n) HEAP32[(((ids)+(i++*4))>>2)] = 0;
          return;
        }
        var id = GL.getNewId(GL.queries);
        query.name = id;
        GL.queries[id] = query;
        HEAP32[(((ids)+(i*4))>>2)] = id;
      }
    };

  var _emscripten_glGenRenderbuffers = (n, renderbuffers) => {
      GL.genObject(n, renderbuffers, 'createRenderbuffer', GL.renderbuffers
        );
    };

  var _emscripten_glGenSamplers = (n, samplers) => {
      GL.genObject(n, samplers, 'createSampler', GL.samplers
        );
    };

  var _emscripten_glGenTextures = (n, textures) => {
      GL.genObject(n, textures, 'createTexture', GL.textures
        );
    };

  var _emscripten_glGenTransformFeedbacks = (n, ids) => {
      GL.genObject(n, ids, 'createTransformFeedback', GL.transformFeedbacks
        );
    };

  var _emscripten_glGenVertexArrays = (n, arrays) => {
      GL.genObject(n, arrays, 'createVertexArray', GL.vaos
        );
    };

  
  var _glGenVertexArrays = _emscripten_glGenVertexArrays;
  var _emscripten_glGenVertexArraysOES = _glGenVertexArrays;

  var _emscripten_glGenerateMipmap = (x0) => GLctx.generateMipmap(x0);

  
  
  var __glGetActiveAttribOrUniform = (funcName, program, index, bufSize, length, size, type, name) => {
      program = GL.programs[program];
      var info = GLctx[funcName](program, index);
      if (info) {
        // If an error occurs, nothing will be written to length, size and type and name.
        var numBytesWrittenExclNull = name && stringToUTF8(info.name, name, bufSize);
        if (length) HEAP32[((length)>>2)] = numBytesWrittenExclNull;
        if (size) HEAP32[((size)>>2)] = info.size;
        if (type) HEAP32[((type)>>2)] = info.type;
      }
    };
  
  var _emscripten_glGetActiveAttrib = (program, index, bufSize, length, size, type, name) =>
      __glGetActiveAttribOrUniform('getActiveAttrib', program, index, bufSize, length, size, type, name);

  
  var _emscripten_glGetActiveUniform = (program, index, bufSize, length, size, type, name) =>
      __glGetActiveAttribOrUniform('getActiveUniform', program, index, bufSize, length, size, type, name);

  var _emscripten_glGetActiveUniformBlockName = (program, uniformBlockIndex, bufSize, length, uniformBlockName) => {
      program = GL.programs[program];
  
      var result = GLctx.getActiveUniformBlockName(program, uniformBlockIndex);
      if (!result) return; // If an error occurs, nothing will be written to uniformBlockName or length.
      if (uniformBlockName && bufSize > 0) {
        var numBytesWrittenExclNull = stringToUTF8(result, uniformBlockName, bufSize);
        if (length) HEAP32[((length)>>2)] = numBytesWrittenExclNull;
      } else {
        if (length) HEAP32[((length)>>2)] = 0;
      }
    };

  var _emscripten_glGetActiveUniformBlockiv = (program, uniformBlockIndex, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if params == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      program = GL.programs[program];
  
      if (pname == 0x8A41 /* GL_UNIFORM_BLOCK_NAME_LENGTH */) {
        var name = GLctx.getActiveUniformBlockName(program, uniformBlockIndex);
        HEAP32[((params)>>2)] = name.length+1;
        return;
      }
  
      var result = GLctx.getActiveUniformBlockParameter(program, uniformBlockIndex, pname);
      if (result === null) return; // If an error occurs, nothing should be written to params.
      if (pname == 0x8A43 /*GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES*/) {
        for (var i = 0; i < result.length; i++) {
          HEAP32[(((params)+(i*4))>>2)] = result[i];
        }
      } else {
        HEAP32[((params)>>2)] = result;
      }
    };

  var _emscripten_glGetActiveUniformsiv = (program, uniformCount, uniformIndices, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if params == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      if (uniformCount > 0 && uniformIndices == 0) {
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      program = GL.programs[program];
      var ids = [];
      for (var i = 0; i < uniformCount; i++) {
        ids.push(HEAP32[(((uniformIndices)+(i*4))>>2)]);
      }
  
      var result = GLctx.getActiveUniforms(program, ids, pname);
      if (!result) return; // GL spec: If an error is generated, nothing is written out to params.
  
      var len = result.length;
      for (var i = 0; i < len; i++) {
        HEAP32[(((params)+(i*4))>>2)] = result[i];
      }
    };

  
  var _emscripten_glGetAttachedShaders = (program, maxCount, count, shaders) => {
      var result = GLctx.getAttachedShaders(GL.programs[program]);
      var len = result.length;
      if (len > maxCount) {
        len = maxCount;
      }
      HEAP32[((count)>>2)] = len;
      for (var i = 0; i < len; ++i) {
        var id = GL.shaders.indexOf(result[i]);
        HEAP32[(((shaders)+(i*4))>>2)] = id;
      }
    };

  
  var _emscripten_glGetAttribLocation = (program, name) =>
      GLctx.getAttribLocation(GL.programs[program], UTF8ToString(name));

  var writeI53ToI64 = (ptr, num) => {
      HEAPU32[((ptr)>>2)] = num;
      var lower = HEAPU32[((ptr)>>2)];
      HEAPU32[(((ptr)+(4))>>2)] = (num - lower)/4294967296;
    };
  
  
  var webglGetExtensions = () => {
      var exts = getEmscriptenSupportedExtensions(GLctx);
      exts = exts.concat(exts.map((e) => 'GL_' + e));
      return exts;
    };
  
  
  
  
  var emscriptenWebGLGet = (name_, p, type) => {
      // Guard against user passing a null pointer.
      // Note that GLES2 spec does not say anything about how passing a null
      // pointer should be treated.  Testing on desktop core GL 3, the application
      // crashes on glGetIntegerv to a null pointer, but better to report an error
      // instead of doing anything random.
      if (!p) {
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      var ret = undefined;
      switch (name_) { // Handle a few trivial GLES values
        case 0x8DFA: // GL_SHADER_COMPILER
          ret = 1;
          break;
        case 0x8DF8: // GL_SHADER_BINARY_FORMATS
          if (type != 0 && type != 1) {
            GL.recordError(0x500); // GL_INVALID_ENUM
          }
          // Do not write anything to the out pointer, since no binary formats are
          // supported.
          return;
        case 0x87FE: // GL_NUM_PROGRAM_BINARY_FORMATS
        case 0x8DF9: // GL_NUM_SHADER_BINARY_FORMATS
          ret = 0;
          break;
        case 0x86A2: // GL_NUM_COMPRESSED_TEXTURE_FORMATS
          // WebGL doesn't have GL_NUM_COMPRESSED_TEXTURE_FORMATS (it's obsolete
          // since GL_COMPRESSED_TEXTURE_FORMATS returns a JS array that can be
          // queried for length), so implement it ourselves to allow C++ GLES2
          // code to get the length.
          var formats = GLctx.getParameter(0x86A3 /*GL_COMPRESSED_TEXTURE_FORMATS*/);
          ret = formats ? formats.length : 0;
          break;
  
        case 0x821D: // GL_NUM_EXTENSIONS
          if (GL.currentContext.version < 2) {
            // Calling GLES3/WebGL2 function with a GLES2/WebGL1 context
            GL.recordError(0x502 /* GL_INVALID_OPERATION */);
            return;
          }
          ret = webglGetExtensions().length;
          break;
        case 0x821B: // GL_MAJOR_VERSION
        case 0x821C: // GL_MINOR_VERSION
          if (GL.currentContext.version < 2) {
            GL.recordError(0x500); // GL_INVALID_ENUM
            return;
          }
          ret = name_ == 0x821B ? 3 : 0; // return version 3.0
          break;
      }
  
      if (ret === undefined) {
        var result = GLctx.getParameter(name_);
        switch (typeof result) {
          case 'number':
            ret = result;
            break;
          case 'boolean':
            ret = result ? 1 : 0;
            break;
          case 'string':
            GL.recordError(0x500); // GL_INVALID_ENUM
            return;
          case 'object':
            if (result === null) {
              // null is a valid result for some (e.g., which buffer is bound -
              // perhaps nothing is bound), but otherwise can mean an invalid
              // name_, which we need to report as an error
              switch (name_) {
                case 0x8894: // ARRAY_BUFFER_BINDING
                case 0x8B8D: // CURRENT_PROGRAM
                case 0x8895: // ELEMENT_ARRAY_BUFFER_BINDING
                case 0x8CA6: // FRAMEBUFFER_BINDING or DRAW_FRAMEBUFFER_BINDING
                case 0x8CA7: // RENDERBUFFER_BINDING
                case 0x8069: // TEXTURE_BINDING_2D
                case 0x85B5: // WebGL 2 GL_VERTEX_ARRAY_BINDING, or WebGL 1 extension OES_vertex_array_object GL_VERTEX_ARRAY_BINDING_OES
                case 0x8F36: // COPY_READ_BUFFER_BINDING or COPY_READ_BUFFER
                case 0x8F37: // COPY_WRITE_BUFFER_BINDING or COPY_WRITE_BUFFER
                case 0x88ED: // PIXEL_PACK_BUFFER_BINDING
                case 0x88EF: // PIXEL_UNPACK_BUFFER_BINDING
                case 0x8CAA: // READ_FRAMEBUFFER_BINDING
                case 0x8919: // SAMPLER_BINDING
                case 0x8C1D: // TEXTURE_BINDING_2D_ARRAY
                case 0x806A: // TEXTURE_BINDING_3D
                case 0x8E25: // TRANSFORM_FEEDBACK_BINDING
                case 0x8C8F: // TRANSFORM_FEEDBACK_BUFFER_BINDING
                case 0x8A28: // UNIFORM_BUFFER_BINDING
                case 0x8514: { // TEXTURE_BINDING_CUBE_MAP
                  ret = 0;
                  break;
                }
                default: {
                  GL.recordError(0x500); // GL_INVALID_ENUM
                  return;
                }
              }
            } else if (result instanceof Float32Array ||
                       result instanceof Uint32Array ||
                       result instanceof Int32Array ||
                       result instanceof Array) {
              for (var i = 0; i < result.length; ++i) {
                switch (type) {
                  case 0: HEAP32[(((p)+(i*4))>>2)] = result[i]; break;
                  case 2: HEAPF32[(((p)+(i*4))>>2)] = result[i]; break;
                  case 4: HEAP8[(p)+(i)] = result[i] ? 1 : 0; break;
                }
              }
              return;
            } else {
              try {
                ret = result.name | 0;
              } catch(e) {
                GL.recordError(0x500); // GL_INVALID_ENUM
                err(`GL_INVALID_ENUM in glGet${type}v: Unknown object returned from WebGL getParameter(${name_})! (error: ${e})`);
                return;
              }
            }
            break;
          default:
            GL.recordError(0x500); // GL_INVALID_ENUM
            err(`GL_INVALID_ENUM in glGet${type}v: Native code calling glGet${type}v(${name_}) and it returns ${result} of type ${typeof(result)}!`);
            return;
        }
      }
  
      switch (type) {
        case 1: writeI53ToI64(p, ret); break;
        case 0: HEAP32[((p)>>2)] = ret; break;
        case 2:   HEAPF32[((p)>>2)] = ret; break;
        case 4: HEAP8[p] = ret ? 1 : 0; break;
      }
    };
  
  var _emscripten_glGetBooleanv = (name_, p) => emscriptenWebGLGet(name_, p, 4);

  var _emscripten_glGetBufferParameteri64v = (target, value, data) => {
      if (!data) {
        // GLES2 specification does not specify how to behave if data is a null pointer. Since calling this function does not make sense
        // if data == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      writeI53ToI64(data, GLctx.getBufferParameter(target, value));
    };

  
  var _emscripten_glGetBufferParameteriv = (target, value, data) => {
      if (!data) {
        // GLES2 specification does not specify how to behave if data is a null
        // pointer. Since calling this function does not make sense if data ==
        // null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      HEAP32[((data)>>2)] = GLctx.getBufferParameter(target, value);
    };

  
  
  var _emscripten_glGetBufferPointerv = (target, pname, params) => {
      if (pname == 0x88BD/*GL_BUFFER_MAP_POINTER*/) {
        var ptr = 0;
        var mappedBuffer = GL.mappedBuffers[emscriptenWebGLGetBufferBinding(target)];
        if (mappedBuffer) {
          ptr = mappedBuffer.mem;
        }
        HEAP32[((params)>>2)] = ptr;
      } else {
        GL.recordError(0x500/*GL_INVALID_ENUM*/);
        err('GL_INVALID_ENUM in glGetBufferPointerv');
      }
    };

  var _emscripten_glGetError = () => {
      var error = GLctx.getError() || GL.lastError;
      GL.lastError = 0/*GL_NO_ERROR*/;
      return error;
    };

  
  var _emscripten_glGetFloatv = (name_, p) => emscriptenWebGLGet(name_, p, 2);

  var _emscripten_glGetFragDataLocation = (program, name) => {
      return GLctx.getFragDataLocation(GL.programs[program], UTF8ToString(name));
    };

  
  var _emscripten_glGetFramebufferAttachmentParameteriv = (target, attachment, pname, params) => {
      var result = GLctx.getFramebufferAttachmentParameter(target, attachment, pname);
      if (result instanceof WebGLRenderbuffer ||
          result instanceof WebGLTexture) {
        result = result.name | 0;
      }
      HEAP32[((params)>>2)] = result;
    };

  
  
  
  var emscriptenWebGLGetIndexed = (target, index, data, type) => {
      if (!data) {
        // GLES2 specification does not specify how to behave if data is a null pointer. Since calling this function does not make sense
        // if data == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      var result = GLctx.getIndexedParameter(target, index);
      var ret;
      switch (typeof result) {
        case 'boolean':
          ret = result ? 1 : 0;
          break;
        case 'number':
          ret = result;
          break;
        case 'object':
          if (result === null) {
            switch (target) {
              case 0x8C8F: // TRANSFORM_FEEDBACK_BUFFER_BINDING
              case 0x8A28: // UNIFORM_BUFFER_BINDING
                ret = 0;
                break;
              default: {
                GL.recordError(0x500); // GL_INVALID_ENUM
                return;
              }
            }
          } else if (result instanceof WebGLBuffer) {
            ret = result.name | 0;
          } else {
            GL.recordError(0x500); // GL_INVALID_ENUM
            return;
          }
          break;
        default:
          GL.recordError(0x500); // GL_INVALID_ENUM
          return;
      }
  
      switch (type) {
        case 1: writeI53ToI64(data, ret); break;
        case 0: HEAP32[((data)>>2)] = ret; break;
        case 2: HEAPF32[((data)>>2)] = ret; break;
        case 4: HEAP8[data] = ret ? 1 : 0; break;
        default: abort('internal emscriptenWebGLGetIndexed() error, bad type: ' + type);
      }
    };
  var _emscripten_glGetInteger64i_v = (target, index, data) =>
      emscriptenWebGLGetIndexed(target, index, data, 1);

  var _emscripten_glGetInteger64v = (name_, p) => {
      emscriptenWebGLGet(name_, p, 1);
    };

  var _emscripten_glGetIntegeri_v = (target, index, data) =>
      emscriptenWebGLGetIndexed(target, index, data, 0);

  
  var _emscripten_glGetIntegerv = (name_, p) => emscriptenWebGLGet(name_, p, 0);

  var _emscripten_glGetInternalformativ = (target, internalformat, pname, bufSize, params) => {
      if (bufSize < 0) {
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      if (!params) {
        // GLES3 specification does not specify how to behave if values is a null pointer. Since calling this function does not make sense
        // if values == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      var ret = GLctx.getInternalformatParameter(target, internalformat, pname);
      if (ret === null) return;
      for (var i = 0; i < ret.length && i < bufSize; ++i) {
        HEAP32[(((params)+(i*4))>>2)] = ret[i];
      }
    };

  var _emscripten_glGetProgramBinary = (program, bufSize, length, binaryFormat, binary) => {
      GL.recordError(0x502/*GL_INVALID_OPERATION*/);
    };

  
  var _emscripten_glGetProgramInfoLog = (program, maxLength, length, infoLog) => {
      var log = GLctx.getProgramInfoLog(GL.programs[program]);
      if (log === null) log = '(unknown error)';
      var numBytesWrittenExclNull = (maxLength > 0 && infoLog) ? stringToUTF8(log, infoLog, maxLength) : 0;
      if (length) HEAP32[((length)>>2)] = numBytesWrittenExclNull;
    };

  
  var _emscripten_glGetProgramiv = (program, pname, p) => {
      if (!p) {
        // GLES2 specification does not specify how to behave if p is a null
        // pointer. Since calling this function does not make sense if p == null,
        // issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
  
      if (program >= GL.counter) {
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
  
      program = GL.programs[program];
  
      if (pname == 0x8B84) { // GL_INFO_LOG_LENGTH
        var log = GLctx.getProgramInfoLog(program);
        if (log === null) log = '(unknown error)';
        HEAP32[((p)>>2)] = log.length + 1;
      } else if (pname == 0x8B87 /* GL_ACTIVE_UNIFORM_MAX_LENGTH */) {
        if (!program.maxUniformLength) {
          var numActiveUniforms = GLctx.getProgramParameter(program, 0x8B86/*GL_ACTIVE_UNIFORMS*/);
          for (var i = 0; i < numActiveUniforms; ++i) {
            program.maxUniformLength = Math.max(program.maxUniformLength, GLctx.getActiveUniform(program, i).name.length+1);
          }
        }
        HEAP32[((p)>>2)] = program.maxUniformLength;
      } else if (pname == 0x8B8A /* GL_ACTIVE_ATTRIBUTE_MAX_LENGTH */) {
        if (!program.maxAttributeLength) {
          var numActiveAttributes = GLctx.getProgramParameter(program, 0x8B89/*GL_ACTIVE_ATTRIBUTES*/);
          for (var i = 0; i < numActiveAttributes; ++i) {
            program.maxAttributeLength = Math.max(program.maxAttributeLength, GLctx.getActiveAttrib(program, i).name.length+1);
          }
        }
        HEAP32[((p)>>2)] = program.maxAttributeLength;
      } else if (pname == 0x8A35 /* GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH */) {
        if (!program.maxUniformBlockNameLength) {
          var numActiveUniformBlocks = GLctx.getProgramParameter(program, 0x8A36/*GL_ACTIVE_UNIFORM_BLOCKS*/);
          for (var i = 0; i < numActiveUniformBlocks; ++i) {
            program.maxUniformBlockNameLength = Math.max(program.maxUniformBlockNameLength, GLctx.getActiveUniformBlockName(program, i).length+1);
          }
        }
        HEAP32[((p)>>2)] = program.maxUniformBlockNameLength;
      } else {
        HEAP32[((p)>>2)] = GLctx.getProgramParameter(program, pname);
      }
    };

  
  var _emscripten_glGetQueryObjecti64vEXT = (id, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if p == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      var query = GL.queries[id];
      var param;
      if (GL.currentContext.version < 2)
      {
        param = GLctx.disjointTimerQueryExt['getQueryObjectEXT'](query, pname);
      }
      else {
        param = GLctx.getQueryParameter(query, pname);
      }
      var ret;
      if (typeof param == 'boolean') {
        ret = param ? 1 : 0;
      } else {
        ret = param;
      }
      writeI53ToI64(params, ret);
    };

  
  var _emscripten_glGetQueryObjectivEXT = (id, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if p == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      var query = GL.queries[id];
      var param = GLctx.disjointTimerQueryExt['getQueryObjectEXT'](query, pname);
      var ret;
      if (typeof param == 'boolean') {
        ret = param ? 1 : 0;
      } else {
        ret = param;
      }
      HEAP32[((params)>>2)] = ret;
    };

  
  var _glGetQueryObjecti64vEXT = _emscripten_glGetQueryObjecti64vEXT;
  var _emscripten_glGetQueryObjectui64vEXT = _glGetQueryObjecti64vEXT;

  var _emscripten_glGetQueryObjectuiv = (id, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if p == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      var query = GL.queries[id];
      var param = GLctx.getQueryParameter(query, pname);
      var ret;
      if (typeof param == 'boolean') {
        ret = param ? 1 : 0;
      } else {
        ret = param;
      }
      HEAP32[((params)>>2)] = ret;
    };

  
  var _glGetQueryObjectivEXT = _emscripten_glGetQueryObjectivEXT;
  var _emscripten_glGetQueryObjectuivEXT = _glGetQueryObjectivEXT;

  var _emscripten_glGetQueryiv = (target, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if p == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      HEAP32[((params)>>2)] = GLctx.getQuery(target, pname);
    };

  
  var _emscripten_glGetQueryivEXT = (target, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if p == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      HEAP32[((params)>>2)] = GLctx.disjointTimerQueryExt['getQueryEXT'](target, pname);
    };

  
  var _emscripten_glGetRenderbufferParameteriv = (target, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if params == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      HEAP32[((params)>>2)] = GLctx.getRenderbufferParameter(target, pname);
    };

  var _emscripten_glGetSamplerParameterfv = (sampler, pname, params) => {
      if (!params) {
        // GLES3 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if p == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      HEAPF32[((params)>>2)] = GLctx.getSamplerParameter(GL.samplers[sampler], pname);
    };

  var _emscripten_glGetSamplerParameteriv = (sampler, pname, params) => {
      if (!params) {
        // GLES3 specification does not specify how to behave if params is a null pointer. Since calling this function does not make sense
        // if p == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      HEAP32[((params)>>2)] = GLctx.getSamplerParameter(GL.samplers[sampler], pname);
    };

  
  
  var _emscripten_glGetShaderInfoLog = (shader, maxLength, length, infoLog) => {
      var log = GLctx.getShaderInfoLog(GL.shaders[shader]);
      if (log === null) log = '(unknown error)';
      var numBytesWrittenExclNull = (maxLength > 0 && infoLog) ? stringToUTF8(log, infoLog, maxLength) : 0;
      if (length) HEAP32[((length)>>2)] = numBytesWrittenExclNull;
    };

  
  var _emscripten_glGetShaderPrecisionFormat = (shaderType, precisionType, range, precision) => {
      var result = GLctx.getShaderPrecisionFormat(shaderType, precisionType);
      HEAP32[((range)>>2)] = result.rangeMin;
      HEAP32[(((range)+(4))>>2)] = result.rangeMax;
      HEAP32[((precision)>>2)] = result.precision;
    };

  
  var _emscripten_glGetShaderSource = (shader, bufSize, length, source) => {
      var result = GLctx.getShaderSource(GL.shaders[shader]);
      if (!result) return; // If an error occurs, nothing will be written to length or source.
      var numBytesWrittenExclNull = (bufSize > 0 && source) ? stringToUTF8(result, source, bufSize) : 0;
      if (length) HEAP32[((length)>>2)] = numBytesWrittenExclNull;
    };

  
  var _emscripten_glGetShaderiv = (shader, pname, p) => {
      if (!p) {
        // GLES2 specification does not specify how to behave if p is a null
        // pointer. Since calling this function does not make sense if p == null,
        // issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      if (pname == 0x8B84) { // GL_INFO_LOG_LENGTH
        var log = GLctx.getShaderInfoLog(GL.shaders[shader]);
        if (log === null) log = '(unknown error)';
        // The GLES2 specification says that if the shader has an empty info log,
        // a value of 0 is returned. Otherwise the log has a null char appended.
        // (An empty string is falsey, so we can just check that instead of
        // looking at log.length.)
        var logLength = log ? log.length + 1 : 0;
        HEAP32[((p)>>2)] = logLength;
      } else if (pname == 0x8B88) { // GL_SHADER_SOURCE_LENGTH
        var source = GLctx.getShaderSource(GL.shaders[shader]);
        // source may be a null, or the empty string, both of which are falsey
        // values that we report a 0 length for.
        var sourceLength = source ? source.length + 1 : 0;
        HEAP32[((p)>>2)] = sourceLength;
      } else {
        HEAP32[((p)>>2)] = GLctx.getShaderParameter(GL.shaders[shader], pname);
      }
    };

  
  
  var _emscripten_glGetString = (name_) => {
      var ret = GL.stringCache[name_];
      if (!ret) {
        switch (name_) {
          case 0x1F03 /* GL_EXTENSIONS */:
            ret = stringToNewUTF8(webglGetExtensions().join(' '));
            break;
          case 0x1F00 /* GL_VENDOR */:
          case 0x1F01 /* GL_RENDERER */:
          case 0x9245 /* UNMASKED_VENDOR_WEBGL */:
          case 0x9246 /* UNMASKED_RENDERER_WEBGL */:
            var s = GLctx.getParameter(name_);
            if (!s) {
              GL.recordError(0x500/*GL_INVALID_ENUM*/);
            }
            ret = s ? stringToNewUTF8(s) : 0;
            break;
  
          case 0x1F02 /* GL_VERSION */:
            var webGLVersion = GLctx.getParameter(0x1F02 /*GL_VERSION*/);
            // return GLES version string corresponding to the version of the WebGL context
            var glVersion = `OpenGL ES 2.0 (${webGLVersion})`;
            if (GL.currentContext.version >= 2) glVersion = `OpenGL ES 3.0 (${webGLVersion})`;
            ret = stringToNewUTF8(glVersion);
            break;
          case 0x8B8C /* GL_SHADING_LANGUAGE_VERSION */:
            var glslVersion = GLctx.getParameter(0x8B8C /*GL_SHADING_LANGUAGE_VERSION*/);
            // extract the version number 'N.M' from the string 'WebGL GLSL ES N.M ...'
            var ver_re = /^WebGL GLSL ES ([0-9]\.[0-9][0-9]?)(?:$| .*)/;
            var ver_num = glslVersion.match(ver_re);
            if (ver_num !== null) {
              if (ver_num[1].length == 3) ver_num[1] = ver_num[1] + '0'; // ensure minor version has 2 digits
              glslVersion = `OpenGL ES GLSL ES ${ver_num[1]} (${glslVersion})`;
            }
            ret = stringToNewUTF8(glslVersion);
            break;
          default:
            GL.recordError(0x500/*GL_INVALID_ENUM*/);
            // fall through
        }
        GL.stringCache[name_] = ret;
      }
      return ret;
    };

  
  var _emscripten_glGetStringi = (name, index) => {
      if (GL.currentContext.version < 2) {
        GL.recordError(0x502 /* GL_INVALID_OPERATION */); // Calling GLES3/WebGL2 function with a GLES2/WebGL1 context
        return 0;
      }
      var stringiCache = GL.stringiCache[name];
      if (stringiCache) {
        if (index < 0 || index >= stringiCache.length) {
          GL.recordError(0x501/*GL_INVALID_VALUE*/);
          return 0;
        }
        return stringiCache[index];
      }
      switch (name) {
        case 0x1F03 /* GL_EXTENSIONS */:
          var exts = webglGetExtensions().map(stringToNewUTF8);
          stringiCache = GL.stringiCache[name] = exts;
          if (index < 0 || index >= stringiCache.length) {
            GL.recordError(0x501/*GL_INVALID_VALUE*/);
            return 0;
          }
          return stringiCache[index];
        default:
          GL.recordError(0x500/*GL_INVALID_ENUM*/);
          return 0;
      }
    };

  var _emscripten_glGetSynciv = (sync, pname, bufSize, length, values) => {
      if (bufSize < 0) {
        // GLES3 specification does not specify how to behave if bufSize < 0, however in the spec wording for glGetInternalformativ, it does say that GL_INVALID_VALUE should be raised,
        // so raise GL_INVALID_VALUE here as well.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      if (!values) {
        // GLES3 specification does not specify how to behave if values is a null pointer. Since calling this function does not make sense
        // if values == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      var ret = GLctx.getSyncParameter(GL.syncs[sync], pname);
      if (ret !== null) {
        HEAP32[((values)>>2)] = ret;
        if (length) HEAP32[((length)>>2)] = 1; // Report a single value outputted.
      }
    };

  
  var _emscripten_glGetTexParameterfv = (target, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null
        // pointer. Since calling this function does not make sense if p == null,
        // issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      HEAPF32[((params)>>2)] = GLctx.getTexParameter(target, pname);
    };

  
  var _emscripten_glGetTexParameteriv = (target, pname, params) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null
        // pointer. Since calling this function does not make sense if p == null,
        // issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      HEAP32[((params)>>2)] = GLctx.getTexParameter(target, pname);
    };

  var _emscripten_glGetTransformFeedbackVarying = (program, index, bufSize, length, size, type, name) => {
      program = GL.programs[program];
      var info = GLctx.getTransformFeedbackVarying(program, index);
      if (!info) return; // If an error occurred, the return parameters length, size, type and name will be unmodified.
  
      if (name && bufSize > 0) {
        var numBytesWrittenExclNull = stringToUTF8(info.name, name, bufSize);
        if (length) HEAP32[((length)>>2)] = numBytesWrittenExclNull;
      } else {
        if (length) HEAP32[((length)>>2)] = 0;
      }
  
      if (size) HEAP32[((size)>>2)] = info.size;
      if (type) HEAP32[((type)>>2)] = info.type;
    };

  var _emscripten_glGetUniformBlockIndex = (program, uniformBlockName) => {
      return GLctx.getUniformBlockIndex(GL.programs[program], UTF8ToString(uniformBlockName));
    };

  
  
  var _emscripten_glGetUniformIndices = (program, uniformCount, uniformNames, uniformIndices) => {
      if (!uniformIndices) {
        // GLES2 specification does not specify how to behave if uniformIndices is a null pointer. Since calling this function does not make sense
        // if uniformIndices == null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      if (uniformCount > 0 && (uniformNames == 0 || uniformIndices == 0)) {
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      program = GL.programs[program];
      var names = [];
      for (var i = 0; i < uniformCount; i++)
        names.push(UTF8ToString(HEAPU32[(((uniformNames)+(i*4))>>2)]));
  
      var result = GLctx.getUniformIndices(program, names);
      if (!result) return; // GL spec: If an error is generated, nothing is written out to uniformIndices.
  
      var len = result.length;
      for (var i = 0; i < len; i++) {
        HEAP32[(((uniformIndices)+(i*4))>>2)] = result[i];
      }
    };

  /** @suppress {checkTypes} */
  var jstoi_q = (str) => parseInt(str);
  
  /** @noinline */
  var webglGetLeftBracePos = (name) => name.slice(-1) == ']' && name.lastIndexOf('[');
  
  var webglPrepareUniformLocationsBeforeFirstUse = (program) => {
      var uniformLocsById = program.uniformLocsById, // Maps GLuint -> WebGLUniformLocation
        uniformSizeAndIdsByName = program.uniformSizeAndIdsByName, // Maps name -> [uniform array length, GLuint]
        i, j;
  
      // On the first time invocation of glGetUniformLocation on this shader program:
      // initialize cache data structures and discover which uniforms are arrays.
      if (!uniformLocsById) {
        // maps GLint integer locations to WebGLUniformLocations
        program.uniformLocsById = uniformLocsById = {};
        // maps integer locations back to uniform name strings, so that we can lazily fetch uniform array locations
        program.uniformArrayNamesById = {};
  
        var numActiveUniforms = GLctx.getProgramParameter(program, 0x8B86/*GL_ACTIVE_UNIFORMS*/);
        for (i = 0; i < numActiveUniforms; ++i) {
          var u = GLctx.getActiveUniform(program, i);
          var nm = u.name;
          var sz = u.size;
          var lb = webglGetLeftBracePos(nm);
          var arrayName = lb > 0 ? nm.slice(0, lb) : nm;
  
          // Assign a new location.
          var id = program.uniformIdCounter;
          program.uniformIdCounter += sz;
          // Eagerly get the location of the uniformArray[0] base element.
          // The remaining indices >0 will be left for lazy evaluation to
          // improve performance. Those may never be needed to fetch, if the
          // application fills arrays always in full starting from the first
          // element of the array.
          uniformSizeAndIdsByName[arrayName] = [sz, id];
  
          // Store placeholder integers in place that highlight that these
          // >0 index locations are array indices pending population.
          for (j = 0; j < sz; ++j) {
            uniformLocsById[id] = j;
            program.uniformArrayNamesById[id++] = arrayName;
          }
        }
      }
    };
  
  
  
  var _emscripten_glGetUniformLocation = (program, name) => {
  
      name = UTF8ToString(name);
  
      if (program = GL.programs[program]) {
        webglPrepareUniformLocationsBeforeFirstUse(program);
        var uniformLocsById = program.uniformLocsById; // Maps GLuint -> WebGLUniformLocation
        var arrayIndex = 0;
        var uniformBaseName = name;
  
        // Invariant: when populating integer IDs for uniform locations, we must
        // maintain the precondition that arrays reside in contiguous addresses,
        // i.e. for a 'vec4 colors[10];', colors[4] must be at location
        // colors[0]+4.  However, user might call glGetUniformLocation(program,
        // "colors") for an array, so we cannot discover based on the user input
        // arguments whether the uniform we are dealing with is an array. The only
        // way to discover which uniforms are arrays is to enumerate over all the
        // active uniforms in the program.
        var leftBrace = webglGetLeftBracePos(name);
  
        // If user passed an array accessor "[index]", parse the array index off the accessor.
        if (leftBrace > 0) {
          arrayIndex = jstoi_q(name.slice(leftBrace + 1)) >>> 0; // "index]", coerce parseInt(']') with >>>0 to treat "foo[]" as "foo[0]" and foo[-1] as unsigned out-of-bounds.
          uniformBaseName = name.slice(0, leftBrace);
        }
  
        // Have we cached the location of this uniform before?
        // A pair [array length, GLint of the uniform location]
        var sizeAndId = program.uniformSizeAndIdsByName[uniformBaseName];
  
        // If a uniform with this name exists, and if its index is within the
        // array limits (if it's even an array), query the WebGLlocation, or
        // return an existing cached location.
        if (sizeAndId && arrayIndex < sizeAndId[0]) {
          arrayIndex += sizeAndId[1]; // Add the base location of the uniform to the array index offset.
          if ((uniformLocsById[arrayIndex] = uniformLocsById[arrayIndex] || GLctx.getUniformLocation(program, name))) {
            return arrayIndex;
          }
        }
      }
      else {
        // N.b. we are currently unable to distinguish between GL program IDs that
        // never existed vs GL program IDs that have been deleted, so report
        // GL_INVALID_VALUE in both cases.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
      }
      return -1;
    };

  
  var webglGetProgramUniformLocation = (program, location) => {
  
      if (program) {
        var webglLoc = program.uniformLocsById[location];
        // program.uniformLocsById[location] stores either an integer, or a
        // WebGLUniformLocation.
        // If an integer, we have not yet bound the location, so do it now. The
        // integer value specifies the array index we should bind to.
        if (typeof webglLoc == 'number') {
          program.uniformLocsById[location] = webglLoc = GLctx.getUniformLocation(program, program.uniformArrayNamesById[location] + (webglLoc > 0 ? `[${webglLoc}]` : ''));
        }
        // Else an already cached WebGLUniformLocation, return it.
        return webglLoc;
      } else {
        GL.recordError(0x502/*GL_INVALID_OPERATION*/);
      }
    };
  
  
  
  
  /** @suppress{checkTypes} */
  var emscriptenWebGLGetUniform = (program, location, params, type) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null
        // pointer. Since calling this function does not make sense if params ==
        // null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      program = GL.programs[program];
      webglPrepareUniformLocationsBeforeFirstUse(program);
      var data = GLctx.getUniform(program, webglGetProgramUniformLocation(program, location));
      if (typeof data == 'number' || typeof data == 'boolean') {
        switch (type) {
          case 0: HEAP32[((params)>>2)] = data; break;
          case 2: HEAPF32[((params)>>2)] = data; break;
        }
      } else {
        for (var i = 0; i < data.length; i++) {
          switch (type) {
            case 0: HEAP32[(((params)+(i*4))>>2)] = data[i]; break;
            case 2: HEAPF32[(((params)+(i*4))>>2)] = data[i]; break;
          }
        }
      }
    };
  
  var _emscripten_glGetUniformfv = (program, location, params) => {
      emscriptenWebGLGetUniform(program, location, params, 2);
    };

  
  var _emscripten_glGetUniformiv = (program, location, params) => {
      emscriptenWebGLGetUniform(program, location, params, 0);
    };

  var _emscripten_glGetUniformuiv = (program, location, params) =>
      emscriptenWebGLGetUniform(program, location, params, 0);

  
  
  /** @suppress{checkTypes} */
  var emscriptenWebGLGetVertexAttrib = (index, pname, params, type) => {
      if (!params) {
        // GLES2 specification does not specify how to behave if params is a null
        // pointer. Since calling this function does not make sense if params ==
        // null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      if (GL.currentContext.clientBuffers[index].enabled) {
        err('glGetVertexAttrib*v on client-side array: not supported, bad data returned');
      }
      var data = GLctx.getVertexAttrib(index, pname);
      if (pname == 0x889F/*VERTEX_ATTRIB_ARRAY_BUFFER_BINDING*/) {
        HEAP32[((params)>>2)] = data && data["name"];
      } else if (typeof data == 'number' || typeof data == 'boolean') {
        switch (type) {
          case 0: HEAP32[((params)>>2)] = data; break;
          case 2: HEAPF32[((params)>>2)] = data; break;
          case 5: HEAP32[((params)>>2)] = Math.fround(data); break;
        }
      } else {
        for (var i = 0; i < data.length; i++) {
          switch (type) {
            case 0: HEAP32[(((params)+(i*4))>>2)] = data[i]; break;
            case 2: HEAPF32[(((params)+(i*4))>>2)] = data[i]; break;
            case 5: HEAP32[(((params)+(i*4))>>2)] = Math.fround(data[i]); break;
          }
        }
      }
    };
  var _emscripten_glGetVertexAttribIiv = (index, pname, params) => {
      // N.B. This function may only be called if the vertex attribute was specified using the function glVertexAttribI4iv(),
      // otherwise the results are undefined. (GLES3 spec 6.1.12)
      emscriptenWebGLGetVertexAttrib(index, pname, params, 0);
    };

  
  var _glGetVertexAttribIiv = _emscripten_glGetVertexAttribIiv;
  var _emscripten_glGetVertexAttribIuiv = _glGetVertexAttribIiv;

  
  var _emscripten_glGetVertexAttribPointerv = (index, pname, pointer) => {
      if (!pointer) {
        // GLES2 specification does not specify how to behave if pointer is a null
        // pointer. Since calling this function does not make sense if pointer ==
        // null, issue a GL error to notify user about it.
        GL.recordError(0x501 /* GL_INVALID_VALUE */);
        return;
      }
      if (GL.currentContext.clientBuffers[index].enabled) {
        err('glGetVertexAttribPointer on client-side array: not supported, bad data returned');
      }
      HEAP32[((pointer)>>2)] = GLctx.getVertexAttribOffset(index, pname);
    };

  
  var _emscripten_glGetVertexAttribfv = (index, pname, params) => {
      // N.B. This function may only be called if the vertex attribute was
      // specified using the function glVertexAttrib*f(), otherwise the results
      // are undefined. (GLES3 spec 6.1.12)
      emscriptenWebGLGetVertexAttrib(index, pname, params, 2);
    };

  
  var _emscripten_glGetVertexAttribiv = (index, pname, params) => {
      // N.B. This function may only be called if the vertex attribute was
      // specified using the function glVertexAttrib*f(), otherwise the results
      // are undefined. (GLES3 spec 6.1.12)
      emscriptenWebGLGetVertexAttrib(index, pname, params, 5);
    };

  var _emscripten_glHint = (x0, x1) => GLctx.hint(x0, x1);

  
  var _emscripten_glInvalidateFramebuffer = (target, numAttachments, attachments) => {
      var list = tempFixedLengthArray[numAttachments];
      for (var i = 0; i < numAttachments; i++) {
        list[i] = HEAP32[(((attachments)+(i*4))>>2)];
      }
  
      GLctx.invalidateFramebuffer(target, list);
    };

  
  var _emscripten_glInvalidateSubFramebuffer = (target, numAttachments, attachments, x, y, width, height) => {
      var list = tempFixedLengthArray[numAttachments];
      for (var i = 0; i < numAttachments; i++) {
        list[i] = HEAP32[(((attachments)+(i*4))>>2)];
      }
  
      GLctx.invalidateSubFramebuffer(target, list, x, y, width, height);
    };

  var _emscripten_glIsBuffer = (buffer) => {
      var b = GL.buffers[buffer];
      if (!b) return 0;
      return GLctx.isBuffer(b);
    };

  var _emscripten_glIsEnabled = (x0) => GLctx.isEnabled(x0);

  var _emscripten_glIsFramebuffer = (framebuffer) => {
      var fb = GL.framebuffers[framebuffer];
      if (!fb) return 0;
      return GLctx.isFramebuffer(fb);
    };

  var _emscripten_glIsProgram = (program) => {
      program = GL.programs[program];
      if (!program) return 0;
      return GLctx.isProgram(program);
    };

  var _emscripten_glIsQuery = (id) => {
      var query = GL.queries[id];
      if (!query) return 0;
      return GLctx.isQuery(query);
    };

  var _emscripten_glIsQueryEXT = (id) => {
      var query = GL.queries[id];
      if (!query) return 0;
      return GLctx.disjointTimerQueryExt['isQueryEXT'](query);
    };

  var _emscripten_glIsRenderbuffer = (renderbuffer) => {
      var rb = GL.renderbuffers[renderbuffer];
      if (!rb) return 0;
      return GLctx.isRenderbuffer(rb);
    };

  var _emscripten_glIsSampler = (id) => {
      var sampler = GL.samplers[id];
      if (!sampler) return 0;
      return GLctx.isSampler(sampler);
    };

  var _emscripten_glIsShader = (shader) => {
      var s = GL.shaders[shader];
      if (!s) return 0;
      return GLctx.isShader(s);
    };

  var _emscripten_glIsSync = (sync) => GLctx.isSync(GL.syncs[sync]);

  var _emscripten_glIsTexture = (id) => {
      var texture = GL.textures[id];
      if (!texture) return 0;
      return GLctx.isTexture(texture);
    };

  var _emscripten_glIsTransformFeedback = (id) => GLctx.isTransformFeedback(GL.transformFeedbacks[id]);

  var _emscripten_glIsVertexArray = (array) => {
  
      var vao = GL.vaos[array];
      if (!vao) return 0;
      return GLctx.isVertexArray(vao);
    };

  
  var _glIsVertexArray = _emscripten_glIsVertexArray;
  var _emscripten_glIsVertexArrayOES = _glIsVertexArray;

  var _emscripten_glLineWidth = (x0) => GLctx.lineWidth(x0);

  var _emscripten_glLinkProgram = (program) => {
      program = GL.programs[program];
      GLctx.linkProgram(program);
      // Invalidate earlier computed uniform->ID mappings, those have now become stale
      program.uniformLocsById = 0; // Mark as null-like so that glGetUniformLocation() knows to populate this again.
      program.uniformSizeAndIdsByName = {};
  
    };

  
  
  
  var _emscripten_glMapBufferRange = (target, offset, length, access) => {
      if ((access & (0x1/*GL_MAP_READ_BIT*/ | 0x20/*GL_MAP_UNSYNCHRONIZED_BIT*/)) != 0) {
        err('glMapBufferRange access does not support MAP_READ or MAP_UNSYNCHRONIZED');
        return 0;
      }
  
      if ((access & 0x2/*GL_MAP_WRITE_BIT*/) == 0) {
        err('glMapBufferRange access must include MAP_WRITE');
        return 0;
      }
  
      if ((access & (0x4/*GL_MAP_INVALIDATE_BUFFER_BIT*/ | 0x8/*GL_MAP_INVALIDATE_RANGE_BIT*/)) == 0) {
        err('glMapBufferRange access must include INVALIDATE_BUFFER or INVALIDATE_RANGE');
        return 0;
      }
  
      if (!emscriptenWebGLValidateMapBufferTarget(target)) {
        GL.recordError(0x500/*GL_INVALID_ENUM*/);
        err('GL_INVALID_ENUM in glMapBufferRange');
        return 0;
      }
  
      var mem = _malloc(length), binding = emscriptenWebGLGetBufferBinding(target);
      if (!mem) return 0;
  
      binding = GL.mappedBuffers[binding] ??= {};
      binding.offset = offset;
      binding.length = length;
      binding.mem = mem;
      binding.access = access;
      return mem;
    };

  var _emscripten_glPauseTransformFeedback = () => GLctx.pauseTransformFeedback();

  var _emscripten_glPixelStorei = (pname, param) => {
      if (pname == 3317) {
        GL.unpackAlignment = param;
      } else if (pname == 3314) {
        GL.unpackRowLength = param;
      }
      GLctx.pixelStorei(pname, param);
    };

  var _emscripten_glPolygonModeWEBGL = (face, mode) => {
      GLctx.webglPolygonMode['polygonModeWEBGL'](face, mode);
    };

  var _emscripten_glPolygonOffset = (x0, x1) => GLctx.polygonOffset(x0, x1);

  var _emscripten_glPolygonOffsetClampEXT = (factor, units, clamp) => {
      GLctx.extPolygonOffsetClamp['polygonOffsetClampEXT'](factor, units, clamp);
    };

  var _emscripten_glProgramBinary = (program, binaryFormat, binary, length) => {
      GL.recordError(0x500/*GL_INVALID_ENUM*/);
    };

  var _emscripten_glProgramParameteri = (program, pname, value) => {
      GL.recordError(0x500/*GL_INVALID_ENUM*/);
    };

  var _emscripten_glQueryCounterEXT = (id, target) => {
      GLctx.disjointTimerQueryExt['queryCounterEXT'](GL.queries[id], target);
    };

  var _emscripten_glReadBuffer = (x0) => GLctx.readBuffer(x0);

  var computeUnpackAlignedImageSize = (width, height, sizePerPixel) => {
      function roundedToNextMultipleOf(x, y) {
        return (x + y - 1) & -y;
      }
      var plainRowSize = (GL.unpackRowLength || width) * sizePerPixel;
      var alignedRowSize = roundedToNextMultipleOf(plainRowSize, GL.unpackAlignment);
      return height * alignedRowSize;
    };
  
  var colorChannelsInGlTextureFormat = (format) => {
      // Micro-optimizations for size: map format to size by subtracting smallest
      // enum value (0x1902) from all values first.  Also omit the most common
      // size value (1) from the list, which is assumed by formats not on the
      // list.
      var colorChannels = {
        // 0x1902 /* GL_DEPTH_COMPONENT */ - 0x1902: 1,
        // 0x1906 /* GL_ALPHA */ - 0x1902: 1,
        5: 3,
        6: 4,
        // 0x1909 /* GL_LUMINANCE */ - 0x1902: 1,
        8: 2,
        29502: 3,
        29504: 4,
        // 0x1903 /* GL_RED */ - 0x1902: 1,
        26917: 2,
        26918: 2,
        // 0x8D94 /* GL_RED_INTEGER */ - 0x1902: 1,
        29846: 3,
        29847: 4
      };
      return colorChannels[format - 0x1902]||1;
    };
  
  
  
  
  
  /** @type {!Uint16Array} */
  var HEAPU16;
  
  
  
  var heapObjectForWebGLType = (type) => {
      // Micro-optimization for size: Subtract lowest GL enum number (0x1400/* GL_BYTE */) from type to compare
      // smaller values for the heap, for shorter generated code size.
      // Also the type HEAPU16 is not tested for explicitly, but any unrecognized type will return out HEAPU16.
      // (since most types are HEAPU16)
      type -= 0x1400;
      if (type == 0) return HEAP8;
  
      if (type == 1) return HEAPU8;
  
      if (type == 2) return HEAP16;
  
      if (type == 4) return HEAP32;
  
      if (type == 6) return HEAPF32;
  
      if (type == 5
        || type == 28922
        || type == 28520
        || type == 30779
        || type == 30782
        )
        return HEAPU32;
  
      return HEAPU16;
    };
  
  var toTypedArrayIndex = (pointer, heap) =>
      pointer >>> (31 - Math.clz32(heap.BYTES_PER_ELEMENT));
  
  var emscriptenWebGLGetTexPixelData = (type, format, width, height, pixels) => {
      var heap = heapObjectForWebGLType(type);
      var sizePerPixel = colorChannelsInGlTextureFormat(format) * heap.BYTES_PER_ELEMENT;
      var bytes = computeUnpackAlignedImageSize(width, height, sizePerPixel);
      return heap.subarray(toTypedArrayIndex(pixels, heap), toTypedArrayIndex(pixels + bytes, heap));
    };
  
  
  
  var _emscripten_glReadPixels = (x, y, width, height, format, type, pixels) => {
      if (GL.currentContext.version >= 2) {
        if (GLctx.currentPixelPackBufferBinding) {
          GLctx.readPixels(x, y, width, height, format, type, pixels);
          return;
        }
        var heap = heapObjectForWebGLType(type);
        var target = toTypedArrayIndex(pixels, heap);
        GLctx.readPixels(x, y, width, height, format, type, heap, target);
        return;
      }
      var pixelData = emscriptenWebGLGetTexPixelData(type, format, width, height, pixels);
      if (!pixelData) {
        GL.recordError(0x500/*GL_INVALID_ENUM*/);
        return;
      }
      GLctx.readPixels(x, y, width, height, format, type, pixelData);
    };

  var _emscripten_glReleaseShaderCompiler = () => {
      // NOP (as allowed by GLES 2.0 spec)
    };

  var _emscripten_glRenderbufferStorage = (x0, x1, x2, x3) => GLctx.renderbufferStorage(x0, x1, x2, x3);

  var _emscripten_glRenderbufferStorageMultisample = (x0, x1, x2, x3, x4) => GLctx.renderbufferStorageMultisample(x0, x1, x2, x3, x4);

  var _emscripten_glResumeTransformFeedback = () => GLctx.resumeTransformFeedback();

  var _emscripten_glSampleCoverage = (value, invert) => {
      GLctx.sampleCoverage(value, !!invert);
    };

  var _emscripten_glSamplerParameterf = (sampler, pname, param) => {
      GLctx.samplerParameterf(GL.samplers[sampler], pname, param);
    };

  var _emscripten_glSamplerParameterfv = (sampler, pname, params) => {
      var param = HEAPF32[((params)>>2)];
      GLctx.samplerParameterf(GL.samplers[sampler], pname, param);
    };

  var _emscripten_glSamplerParameteri = (sampler, pname, param) => {
      GLctx.samplerParameteri(GL.samplers[sampler], pname, param);
    };

  var _emscripten_glSamplerParameteriv = (sampler, pname, params) => {
      var param = HEAP32[((params)>>2)];
      GLctx.samplerParameteri(GL.samplers[sampler], pname, param);
    };

  var _emscripten_glScissor = (x0, x1, x2, x3) => GLctx.scissor(x0, x1, x2, x3);

  var _emscripten_glShaderBinary = (count, shaders, binaryformat, binary, length) => {
      GL.recordError(0x500/*GL_INVALID_ENUM*/);
    };

  var _emscripten_glShaderSource = (shader, count, string, length) => {
      var source = GL.getSource(shader, count, string, length);
  
      GLctx.shaderSource(GL.shaders[shader], source);
    };

  var _emscripten_glStencilFunc = (x0, x1, x2) => GLctx.stencilFunc(x0, x1, x2);

  var _emscripten_glStencilFuncSeparate = (x0, x1, x2, x3) => GLctx.stencilFuncSeparate(x0, x1, x2, x3);

  var _emscripten_glStencilMask = (x0) => GLctx.stencilMask(x0);

  var _emscripten_glStencilMaskSeparate = (x0, x1) => GLctx.stencilMaskSeparate(x0, x1);

  var _emscripten_glStencilOp = (x0, x1, x2) => GLctx.stencilOp(x0, x1, x2);

  var _emscripten_glStencilOpSeparate = (x0, x1, x2, x3) => GLctx.stencilOpSeparate(x0, x1, x2, x3);

  
  
  
  var _emscripten_glTexImage2D = (target, level, internalFormat, width, height, border, format, type, pixels) => {
      if (GL.currentContext.version >= 2) {
        if (GLctx.currentPixelUnpackBufferBinding) {
          GLctx.texImage2D(target, level, internalFormat, width, height, border, format, type, pixels);
          return;
        }
        if (pixels) {
          var heap = heapObjectForWebGLType(type);
          var index = toTypedArrayIndex(pixels, heap);
          GLctx.texImage2D(target, level, internalFormat, width, height, border, format, type, heap, index);
          return;
        }
      }
      var pixelData = pixels ? emscriptenWebGLGetTexPixelData(type, format, width, height, pixels) : null;
      GLctx.texImage2D(target, level, internalFormat, width, height, border, format, type, pixelData);
    };

  
  var _emscripten_glTexImage3D = (target, level, internalFormat, width, height, depth, border, format, type, pixels) => {
      if (GLctx.currentPixelUnpackBufferBinding) {
        GLctx.texImage3D(target, level, internalFormat, width, height, depth, border, format, type, pixels);
      } else if (pixels) {
        var heap = heapObjectForWebGLType(type);
        GLctx.texImage3D(target, level, internalFormat, width, height, depth, border, format, type, heap, toTypedArrayIndex(pixels, heap));
      } else {
        GLctx.texImage3D(target, level, internalFormat, width, height, depth, border, format, type, null);
      }
    };

  var _emscripten_glTexParameterf = (x0, x1, x2) => GLctx.texParameterf(x0, x1, x2);

  
  var _emscripten_glTexParameterfv = (target, pname, params) => {
      var param = HEAPF32[((params)>>2)];
      GLctx.texParameterf(target, pname, param);
    };

  var _emscripten_glTexParameteri = (x0, x1, x2) => GLctx.texParameteri(x0, x1, x2);

  
  var _emscripten_glTexParameteriv = (target, pname, params) => {
      var param = HEAP32[((params)>>2)];
      GLctx.texParameteri(target, pname, param);
    };

  var _emscripten_glTexStorage2D = (x0, x1, x2, x3, x4) => GLctx.texStorage2D(x0, x1, x2, x3, x4);

  var _emscripten_glTexStorage3D = (x0, x1, x2, x3, x4, x5) => GLctx.texStorage3D(x0, x1, x2, x3, x4, x5);

  
  
  
  var _emscripten_glTexSubImage2D = (target, level, xoffset, yoffset, width, height, format, type, pixels) => {
      if (GL.currentContext.version >= 2) {
        if (GLctx.currentPixelUnpackBufferBinding) {
          GLctx.texSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
          return;
        }
        if (pixels) {
          var heap = heapObjectForWebGLType(type);
          GLctx.texSubImage2D(target, level, xoffset, yoffset, width, height, format, type, heap, toTypedArrayIndex(pixels, heap));
          return;
        }
      }
      var pixelData = pixels ? emscriptenWebGLGetTexPixelData(type, format, width, height, pixels) : null;
      GLctx.texSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixelData);
    };

  
  var _emscripten_glTexSubImage3D = (target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels) => {
      if (GLctx.currentPixelUnpackBufferBinding) {
        GLctx.texSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
      } else if (pixels) {
        var heap = heapObjectForWebGLType(type);
        GLctx.texSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, heap, toTypedArrayIndex(pixels, heap));
      } else {
        GLctx.texSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, null);
      }
    };

  
  var _emscripten_glTransformFeedbackVaryings = (program, count, varyings, bufferMode) => {
      program = GL.programs[program];
      var vars = [];
      for (var i = 0; i < count; i++)
        vars.push(UTF8ToString(HEAPU32[(((varyings)+(i*4))>>2)]));
  
      GLctx.transformFeedbackVaryings(program, vars, bufferMode);
    };

  
  var webglGetUniformLocation = (location) => {
  
      return webglGetProgramUniformLocation(GLctx.currentProgram, location);
    };
  
  var _emscripten_glUniform1f = (location, v0) => {
      GLctx.uniform1f(webglGetUniformLocation(location), v0);
    };

  
  var miniTempWebGLFloatBuffers = [];
  
  
  var _emscripten_glUniform1fv = (location, count, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniform1fv(webglGetUniformLocation(location), HEAPF32, ((value)>>2), count);
        return;
      }
  
      if (count <= 288) {
        // avoid allocation when uploading few enough uniforms
        var view = miniTempWebGLFloatBuffers[count];
        for (var i = 0; i < count; ++i) {
          view[i] = HEAPF32[(((value)+(4*i))>>2)];
        }
      } else
      {
        var view = HEAPF32.subarray((((value)>>2)), ((value+count*4)>>2));
      }
      GLctx.uniform1fv(webglGetUniformLocation(location), view);
    };

  
  var _emscripten_glUniform1i = (location, v0) => {
      GLctx.uniform1i(webglGetUniformLocation(location), v0);
    };

  
  var miniTempWebGLIntBuffers = [];
  
  
  var _emscripten_glUniform1iv = (location, count, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniform1iv(webglGetUniformLocation(location), HEAP32, ((value)>>2), count);
        return;
      }
  
      if (count <= 288) {
        // avoid allocation when uploading few enough uniforms
        var view = miniTempWebGLIntBuffers[count];
        for (var i = 0; i < count; ++i) {
          view[i] = HEAP32[(((value)+(4*i))>>2)];
        }
      } else
      {
        var view = HEAP32.subarray((((value)>>2)), ((value+count*4)>>2));
      }
      GLctx.uniform1iv(webglGetUniformLocation(location), view);
    };

  var _emscripten_glUniform1ui = (location, v0) => {
      GLctx.uniform1ui(webglGetUniformLocation(location), v0);
    };

  
  var _emscripten_glUniform1uiv = (location, count, value) => {
      count && GLctx.uniform1uiv(webglGetUniformLocation(location), HEAPU32, ((value)>>2), count);
    };

  
  var _emscripten_glUniform2f = (location, v0, v1) => {
      GLctx.uniform2f(webglGetUniformLocation(location), v0, v1);
    };

  
  
  
  var _emscripten_glUniform2fv = (location, count, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniform2fv(webglGetUniformLocation(location), HEAPF32, ((value)>>2), count*2);
        return;
      }
  
      if (count <= 144) {
        // avoid allocation when uploading few enough uniforms
        count *= 2;
        var view = miniTempWebGLFloatBuffers[count];
        for (var i = 0; i < count; i += 2) {
          view[i] = HEAPF32[(((value)+(4*i))>>2)];
          view[i+1] = HEAPF32[(((value)+(4*i+4))>>2)];
        }
      } else
      {
        var view = HEAPF32.subarray((((value)>>2)), ((value+count*8)>>2));
      }
      GLctx.uniform2fv(webglGetUniformLocation(location), view);
    };

  
  var _emscripten_glUniform2i = (location, v0, v1) => {
      GLctx.uniform2i(webglGetUniformLocation(location), v0, v1);
    };

  
  
  
  var _emscripten_glUniform2iv = (location, count, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniform2iv(webglGetUniformLocation(location), HEAP32, ((value)>>2), count*2);
        return;
      }
  
      if (count <= 144) {
        // avoid allocation when uploading few enough uniforms
        count *= 2;
        var view = miniTempWebGLIntBuffers[count];
        for (var i = 0; i < count; i += 2) {
          view[i] = HEAP32[(((value)+(4*i))>>2)];
          view[i+1] = HEAP32[(((value)+(4*i+4))>>2)];
        }
      } else
      {
        var view = HEAP32.subarray((((value)>>2)), ((value+count*8)>>2));
      }
      GLctx.uniform2iv(webglGetUniformLocation(location), view);
    };

  var _emscripten_glUniform2ui = (location, v0, v1) => {
      GLctx.uniform2ui(webglGetUniformLocation(location), v0, v1);
    };

  
  var _emscripten_glUniform2uiv = (location, count, value) => {
      count && GLctx.uniform2uiv(webglGetUniformLocation(location), HEAPU32, ((value)>>2), count*2);
    };

  
  var _emscripten_glUniform3f = (location, v0, v1, v2) => {
      GLctx.uniform3f(webglGetUniformLocation(location), v0, v1, v2);
    };

  
  
  
  var _emscripten_glUniform3fv = (location, count, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniform3fv(webglGetUniformLocation(location), HEAPF32, ((value)>>2), count*3);
        return;
      }
  
      if (count <= 96) {
        // avoid allocation when uploading few enough uniforms
        count *= 3;
        var view = miniTempWebGLFloatBuffers[count];
        for (var i = 0; i < count; i += 3) {
          view[i] = HEAPF32[(((value)+(4*i))>>2)];
          view[i+1] = HEAPF32[(((value)+(4*i+4))>>2)];
          view[i+2] = HEAPF32[(((value)+(4*i+8))>>2)];
        }
      } else
      {
        var view = HEAPF32.subarray((((value)>>2)), ((value+count*12)>>2));
      }
      GLctx.uniform3fv(webglGetUniformLocation(location), view);
    };

  
  var _emscripten_glUniform3i = (location, v0, v1, v2) => {
      GLctx.uniform3i(webglGetUniformLocation(location), v0, v1, v2);
    };

  
  
  
  var _emscripten_glUniform3iv = (location, count, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniform3iv(webglGetUniformLocation(location), HEAP32, ((value)>>2), count*3);
        return;
      }
  
      if (count <= 96) {
        // avoid allocation when uploading few enough uniforms
        count *= 3;
        var view = miniTempWebGLIntBuffers[count];
        for (var i = 0; i < count; i += 3) {
          view[i] = HEAP32[(((value)+(4*i))>>2)];
          view[i+1] = HEAP32[(((value)+(4*i+4))>>2)];
          view[i+2] = HEAP32[(((value)+(4*i+8))>>2)];
        }
      } else
      {
        var view = HEAP32.subarray((((value)>>2)), ((value+count*12)>>2));
      }
      GLctx.uniform3iv(webglGetUniformLocation(location), view);
    };

  var _emscripten_glUniform3ui = (location, v0, v1, v2) => {
      GLctx.uniform3ui(webglGetUniformLocation(location), v0, v1, v2);
    };

  
  var _emscripten_glUniform3uiv = (location, count, value) => {
      count && GLctx.uniform3uiv(webglGetUniformLocation(location), HEAPU32, ((value)>>2), count*3);
    };

  
  var _emscripten_glUniform4f = (location, v0, v1, v2, v3) => {
      GLctx.uniform4f(webglGetUniformLocation(location), v0, v1, v2, v3);
    };

  
  
  
  var _emscripten_glUniform4fv = (location, count, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniform4fv(webglGetUniformLocation(location), HEAPF32, ((value)>>2), count*4);
        return;
      }
  
      if (count <= 72) {
        // avoid allocation when uploading few enough uniforms
        var view = miniTempWebGLFloatBuffers[4*count];
        // hoist the heap out of the loop for size and for pthreads+growth.
        var heap = HEAPF32;
        value = ((value)>>2);
        count *= 4;
        for (var i = 0; i < count; i += 4) {
          var dst = value + i;
          view[i] = heap[dst];
          view[i + 1] = heap[dst + 1];
          view[i + 2] = heap[dst + 2];
          view[i + 3] = heap[dst + 3];
        }
      } else
      {
        var view = HEAPF32.subarray((((value)>>2)), ((value+count*16)>>2));
      }
      GLctx.uniform4fv(webglGetUniformLocation(location), view);
    };

  
  var _emscripten_glUniform4i = (location, v0, v1, v2, v3) => {
      GLctx.uniform4i(webglGetUniformLocation(location), v0, v1, v2, v3);
    };

  
  
  
  var _emscripten_glUniform4iv = (location, count, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniform4iv(webglGetUniformLocation(location), HEAP32, ((value)>>2), count*4);
        return;
      }
  
      if (count <= 72) {
        // avoid allocation when uploading few enough uniforms
        count *= 4;
        var view = miniTempWebGLIntBuffers[count];
        for (var i = 0; i < count; i += 4) {
          view[i] = HEAP32[(((value)+(4*i))>>2)];
          view[i+1] = HEAP32[(((value)+(4*i+4))>>2)];
          view[i+2] = HEAP32[(((value)+(4*i+8))>>2)];
          view[i+3] = HEAP32[(((value)+(4*i+12))>>2)];
        }
      } else
      {
        var view = HEAP32.subarray((((value)>>2)), ((value+count*16)>>2));
      }
      GLctx.uniform4iv(webglGetUniformLocation(location), view);
    };

  var _emscripten_glUniform4ui = (location, v0, v1, v2, v3) => {
      GLctx.uniform4ui(webglGetUniformLocation(location), v0, v1, v2, v3);
    };

  
  var _emscripten_glUniform4uiv = (location, count, value) => {
      count && GLctx.uniform4uiv(webglGetUniformLocation(location), HEAPU32, ((value)>>2), count*4);
    };

  var _emscripten_glUniformBlockBinding = (program, uniformBlockIndex, uniformBlockBinding) => {
      program = GL.programs[program];
  
      GLctx.uniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
    };

  
  
  
  var _emscripten_glUniformMatrix2fv = (location, count, transpose, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniformMatrix2fv(webglGetUniformLocation(location), !!transpose, HEAPF32, ((value)>>2), count*4);
        return;
      }
  
      if (count <= 72) {
        // avoid allocation when uploading few enough uniforms
        count *= 4;
        var view = miniTempWebGLFloatBuffers[count];
        for (var i = 0; i < count; i += 4) {
          view[i] = HEAPF32[(((value)+(4*i))>>2)];
          view[i+1] = HEAPF32[(((value)+(4*i+4))>>2)];
          view[i+2] = HEAPF32[(((value)+(4*i+8))>>2)];
          view[i+3] = HEAPF32[(((value)+(4*i+12))>>2)];
        }
      } else
      {
        var view = HEAPF32.subarray((((value)>>2)), ((value+count*16)>>2));
      }
      GLctx.uniformMatrix2fv(webglGetUniformLocation(location), !!transpose, view);
    };

  
  var _emscripten_glUniformMatrix2x3fv = (location, count, transpose, value) => {
      count && GLctx.uniformMatrix2x3fv(webglGetUniformLocation(location), !!transpose, HEAPF32, ((value)>>2), count*6);
    };

  
  var _emscripten_glUniformMatrix2x4fv = (location, count, transpose, value) => {
      count && GLctx.uniformMatrix2x4fv(webglGetUniformLocation(location), !!transpose, HEAPF32, ((value)>>2), count*8);
    };

  
  
  
  var _emscripten_glUniformMatrix3fv = (location, count, transpose, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniformMatrix3fv(webglGetUniformLocation(location), !!transpose, HEAPF32, ((value)>>2), count*9);
        return;
      }
  
      if (count <= 32) {
        // avoid allocation when uploading few enough uniforms
        count *= 9;
        var view = miniTempWebGLFloatBuffers[count];
        for (var i = 0; i < count; i += 9) {
          view[i] = HEAPF32[(((value)+(4*i))>>2)];
          view[i+1] = HEAPF32[(((value)+(4*i+4))>>2)];
          view[i+2] = HEAPF32[(((value)+(4*i+8))>>2)];
          view[i+3] = HEAPF32[(((value)+(4*i+12))>>2)];
          view[i+4] = HEAPF32[(((value)+(4*i+16))>>2)];
          view[i+5] = HEAPF32[(((value)+(4*i+20))>>2)];
          view[i+6] = HEAPF32[(((value)+(4*i+24))>>2)];
          view[i+7] = HEAPF32[(((value)+(4*i+28))>>2)];
          view[i+8] = HEAPF32[(((value)+(4*i+32))>>2)];
        }
      } else
      {
        var view = HEAPF32.subarray((((value)>>2)), ((value+count*36)>>2));
      }
      GLctx.uniformMatrix3fv(webglGetUniformLocation(location), !!transpose, view);
    };

  
  var _emscripten_glUniformMatrix3x2fv = (location, count, transpose, value) => {
      count && GLctx.uniformMatrix3x2fv(webglGetUniformLocation(location), !!transpose, HEAPF32, ((value)>>2), count*6);
    };

  
  var _emscripten_glUniformMatrix3x4fv = (location, count, transpose, value) => {
      count && GLctx.uniformMatrix3x4fv(webglGetUniformLocation(location), !!transpose, HEAPF32, ((value)>>2), count*12);
    };

  
  
  
  var _emscripten_glUniformMatrix4fv = (location, count, transpose, value) => {
  
      if (GL.currentContext.version >= 2) {
        count && GLctx.uniformMatrix4fv(webglGetUniformLocation(location), !!transpose, HEAPF32, ((value)>>2), count*16);
        return;
      }
  
      if (count <= 18) {
        // avoid allocation when uploading few enough uniforms
        var view = miniTempWebGLFloatBuffers[16*count];
        // hoist the heap out of the loop for size and for pthreads+growth.
        var heap = HEAPF32;
        value = ((value)>>2);
        count *= 16;
        for (var i = 0; i < count; i += 16) {
          var dst = value + i;
          view[i] = heap[dst];
          view[i + 1] = heap[dst + 1];
          view[i + 2] = heap[dst + 2];
          view[i + 3] = heap[dst + 3];
          view[i + 4] = heap[dst + 4];
          view[i + 5] = heap[dst + 5];
          view[i + 6] = heap[dst + 6];
          view[i + 7] = heap[dst + 7];
          view[i + 8] = heap[dst + 8];
          view[i + 9] = heap[dst + 9];
          view[i + 10] = heap[dst + 10];
          view[i + 11] = heap[dst + 11];
          view[i + 12] = heap[dst + 12];
          view[i + 13] = heap[dst + 13];
          view[i + 14] = heap[dst + 14];
          view[i + 15] = heap[dst + 15];
        }
      } else
      {
        var view = HEAPF32.subarray((((value)>>2)), ((value+count*64)>>2));
      }
      GLctx.uniformMatrix4fv(webglGetUniformLocation(location), !!transpose, view);
    };

  
  var _emscripten_glUniformMatrix4x2fv = (location, count, transpose, value) => {
      count && GLctx.uniformMatrix4x2fv(webglGetUniformLocation(location), !!transpose, HEAPF32, ((value)>>2), count*8);
    };

  
  var _emscripten_glUniformMatrix4x3fv = (location, count, transpose, value) => {
      count && GLctx.uniformMatrix4x3fv(webglGetUniformLocation(location), !!transpose, HEAPF32, ((value)>>2), count*12);
    };

  
  
  
  var _emscripten_glUnmapBuffer = (target) => {
      if (!emscriptenWebGLValidateMapBufferTarget(target)) {
        GL.recordError(0x500/*GL_INVALID_ENUM*/);
        err('GL_INVALID_ENUM in glUnmapBuffer');
        return 0;
      }
  
      var buffer = emscriptenWebGLGetBufferBinding(target);
      var mapping = GL.mappedBuffers[buffer];
      if (!mapping || !mapping.mem) {
        GL.recordError(0x502 /* GL_INVALID_OPERATION */);
        err('buffer was never mapped in glUnmapBuffer');
        return 0;
      }
  
      if (!(mapping.access & 0x10)) { /* GL_MAP_FLUSH_EXPLICIT_BIT */
        webglBufferSubData(target, mapping.offset, mapping.length, mapping.mem);
      }
  
      _free(mapping.mem);
      mapping.mem = 0;
      return 1;
    };

  var _emscripten_glUseProgram = (program) => {
      program = GL.programs[program];
      GLctx.useProgram(program);
      // Record the currently active program so that we can access the uniform
      // mapping table of that program.
      GLctx.currentProgram = program;
    };

  var _emscripten_glValidateProgram = (program) => {
      GLctx.validateProgram(GL.programs[program]);
    };

  var _emscripten_glVertexAttrib1f = (x0, x1) => GLctx.vertexAttrib1f(x0, x1);

  
  var _emscripten_glVertexAttrib1fv = (index, v) => {
  
      GLctx.vertexAttrib1f(index, HEAPF32[v>>2]);
    };

  var _emscripten_glVertexAttrib2f = (x0, x1, x2) => GLctx.vertexAttrib2f(x0, x1, x2);

  
  var _emscripten_glVertexAttrib2fv = (index, v) => {
  
      GLctx.vertexAttrib2f(index, HEAPF32[v>>2], HEAPF32[v+4>>2]);
    };

  var _emscripten_glVertexAttrib3f = (x0, x1, x2, x3) => GLctx.vertexAttrib3f(x0, x1, x2, x3);

  
  var _emscripten_glVertexAttrib3fv = (index, v) => {
  
      GLctx.vertexAttrib3f(index, HEAPF32[v>>2], HEAPF32[v+4>>2], HEAPF32[v+8>>2]);
    };

  var _emscripten_glVertexAttrib4f = (x0, x1, x2, x3, x4) => GLctx.vertexAttrib4f(x0, x1, x2, x3, x4);

  
  var _emscripten_glVertexAttrib4fv = (index, v) => {
  
      GLctx.vertexAttrib4f(index, HEAPF32[v>>2], HEAPF32[v+4>>2], HEAPF32[v+8>>2], HEAPF32[v+12>>2]);
    };

  var _emscripten_glVertexAttribDivisor = (index, divisor) => {
      GLctx.vertexAttribDivisor(index, divisor);
    };

  
  var _glVertexAttribDivisor = _emscripten_glVertexAttribDivisor;
  var _emscripten_glVertexAttribDivisorANGLE = _glVertexAttribDivisor;

  
  var _emscripten_glVertexAttribDivisorARB = _glVertexAttribDivisor;

  
  var _emscripten_glVertexAttribDivisorEXT = _glVertexAttribDivisor;

  
  var _emscripten_glVertexAttribDivisorNV = _glVertexAttribDivisor;

  var _emscripten_glVertexAttribI4i = (x0, x1, x2, x3, x4) => GLctx.vertexAttribI4i(x0, x1, x2, x3, x4);

  var _emscripten_glVertexAttribI4iv = (index, v) => {
      GLctx.vertexAttribI4i(index, HEAP32[v>>2], HEAP32[v+4>>2], HEAP32[v+8>>2], HEAP32[v+12>>2]);
    };

  var _emscripten_glVertexAttribI4ui = (x0, x1, x2, x3, x4) => GLctx.vertexAttribI4ui(x0, x1, x2, x3, x4);

  var _emscripten_glVertexAttribI4uiv = (index, v) => {
      GLctx.vertexAttribI4ui(index, HEAPU32[v>>2], HEAPU32[v+4>>2], HEAPU32[v+8>>2], HEAPU32[v+12>>2]);
    };

  var _emscripten_glVertexAttribIPointer = (index, size, type, stride, ptr) => {
      var cb = GL.currentContext.clientBuffers[index];
      if (!GLctx.currentArrayBufferBinding) {
        cb.size = size;
        cb.type = type;
        cb.normalized = false;
        cb.stride = stride;
        cb.ptr = ptr;
        cb.clientside = true;
        cb.vertexAttribPointerAdaptor = /** @this {WebGLRenderingContext} */ function(index, size, type, normalized, stride, ptr) {
          this.vertexAttribIPointer(index, size, type, stride, ptr);
        };
        return;
      }
      cb.clientside = false;
      GLctx.vertexAttribIPointer(index, size, type, stride, ptr);
    };

  var _emscripten_glVertexAttribPointer = (index, size, type, normalized, stride, ptr) => {
      var cb = GL.currentContext.clientBuffers[index];
      if (!GLctx.currentArrayBufferBinding) {
        cb.size = size;
        cb.type = type;
        cb.normalized = normalized;
        cb.stride = stride;
        cb.ptr = ptr;
        cb.clientside = true;
        cb.vertexAttribPointerAdaptor = /** @this {WebGLRenderingContext} */ function(index, size, type, normalized, stride, ptr) {
          this.vertexAttribPointer(index, size, type, normalized, stride, ptr);
        };
        return;
      }
      cb.clientside = false;
      GLctx.vertexAttribPointer(index, size, type, !!normalized, stride, ptr);
    };

  var _emscripten_glViewport = (x0, x1, x2, x3) => GLctx.viewport(x0, x1, x2, x3);

  var _emscripten_glWaitSync = (sync, flags, timeout) => {
      // See WebGL2 vs GLES3 difference on GL_TIMEOUT_IGNORED above (https://www.khronos.org/registry/webgl/specs/latest/2.0/#5.15)
      timeout = Number(timeout);
      GLctx.waitSync(GL.syncs[sync], flags, timeout);
    };

  var getHeapMax = () =>
      // Stay one Wasm page short of 4GB: while e.g. Chrome is able to allocate
      // full 4GB Wasm memories, the size will wrap back to 0 bytes in Wasm side
      // for any code that deals with heap sizes, which would require special
      // casing all heap size related code to treat 0 specially.
      2147483648;
  
  var alignMemory = (size, alignment) => {
      return Math.ceil(size / alignment) * alignment;
    };
  
  var growMemory = (size) => {
      var oldHeapSize = wasmMemory.buffer.byteLength;
      var pages = ((size - oldHeapSize + 65535) / 65536) | 0;
      try {
        // round size grow request up to wasm page size (fixed 64KB per spec)
        wasmMemory.grow(pages); // .grow() takes a delta compared to the previous size
        updateMemoryViews();
        return 1 /*success*/;
      } catch(e) {
      }
      // implicit 0 return to save code size (caller will cast 'undefined' into 0
      // anyhow)
    };
  
  var _emscripten_resize_heap = (requestedSize) => {
      var oldSize = HEAPU8.length;
      // With CAN_ADDRESS_2GB or MEMORY64, pointers are already unsigned.
      requestedSize >>>= 0;
      // With multithreaded builds, races can happen (another thread might increase the size
      // in between), so return a failure, and let the caller retry.
  
      // Memory resize rules:
      // 1.  Always increase heap size to at least the requested size, rounded up
      //     to next page multiple.
      // 2a. If MEMORY_GROWTH_LINEAR_STEP == -1, excessively resize the heap
      //     geometrically: increase the heap size according to
      //     MEMORY_GROWTH_GEOMETRIC_STEP factor (default +20%), At most
      //     overreserve by MEMORY_GROWTH_GEOMETRIC_CAP bytes (default 96MB).
      // 2b. If MEMORY_GROWTH_LINEAR_STEP != -1, excessively resize the heap
      //     linearly: increase the heap size by at least
      //     MEMORY_GROWTH_LINEAR_STEP bytes.
      // 3.  Max size for the heap is capped at 2048MB-WASM_PAGE_SIZE, or by
      //     MAXIMUM_MEMORY, or by ASAN limit, depending on which is smallest
      // 4.  If we were unable to allocate as much memory, it may be due to
      //     over-eager decision to excessively reserve due to (3) above.
      //     Hence if an allocation fails, cut down on the amount of excess
      //     growth, in an attempt to succeed to perform a smaller allocation.
  
      // A limit is set for how much we can grow. We should not exceed that
      // (the wasm binary specifies it, so if we tried, we'd fail anyhow).
      var maxHeapSize = getHeapMax();
      if (requestedSize > maxHeapSize) {
        return false;
      }
  
      // Loop through potential heap size increases. If we attempt a too eager
      // reservation that fails, cut down on the attempted size and reserve a
      // smaller bump instead. (max 3 times, chosen somewhat arbitrarily)
      for (var cutDown = 1; cutDown <= 4; cutDown *= 2) {
        var overGrownHeapSize = oldSize * (1 + 0.2 / cutDown); // ensure geometric growth
        // but limit overreserving (default to capping at +96MB overgrowth at most)
        overGrownHeapSize = Math.min(overGrownHeapSize, requestedSize + 100663296 );
  
        var newSize = Math.min(maxHeapSize, alignMemory(Math.max(requestedSize, overGrownHeapSize), 65536));
  
        var replacement = growMemory(newSize);
        if (replacement) {
  
          return true;
        }
      }
      return false;
    };

  /** @suppress {checkTypes} */
  var _emscripten_sample_gamepad_data = () => {
      try {
        if (navigator.getGamepads) return (JSEvents.lastGamepadState = navigator.getGamepads())
          ? 0 : -1;
      } catch(e) {
        navigator.getGamepads = null; // Disable getGamepads() so that it won't be attempted to be used again.
      }
      return -1;
    };

  var maybeCStringToJsString = (cString) => {
      // 'cString > 2' checks if the input is a number, and isn't of the special
      // values we accept here, EMSCRIPTEN_EVENT_TARGET_* (which map to 0, 1, 2).
      // In other words, if cString > 2 then it's a pointer to a valid place in
      // memory, and points to a C string.
      return cString > 2 ? UTF8ToString(cString) : cString;
    };
  
  /** @type {Object} */
  var specialHTMLTargets = [0, globalThis.document ?? 0, globalThis.window ?? 0];
  var findEventTarget = (target) => {
      target = maybeCStringToJsString(target);
      var domElement = specialHTMLTargets[target] || globalThis.document?.querySelector(target);
      return domElement;
    };
  var findCanvasEventTarget = findEventTarget;
  var _emscripten_set_canvas_element_size = (target, width, height) => {
      var canvas = findCanvasEventTarget(target);
      if (!canvas) return -4;
      canvas.width = width;
      canvas.height = height;
      return 0;
    };

  
  var _emscripten_set_main_loop = (func, fps, simulateInfiniteLoop) => {
      var iterFunc = getWasmTableEntry(func);
      setMainLoop(iterFunc, fps, simulateInfiniteLoop);
    };

  
  
  
  
  
  
  
  
  
  
  var _emscripten_webgl_enable_extension = (contextHandle, extension) => {
      var context = GL.getContext(contextHandle);
      var extString = UTF8ToString(extension);
      if (extString.startsWith('GL_')) extString = extString.slice(3); // Allow enabling extensions both with "GL_" prefix and without.
  
      // Switch-board that pulls in code for all GL extensions, even if those are not used :/
      // Build with -sGL_SUPPORT_SIMPLE_ENABLE_EXTENSIONS=0 to avoid this.
  
      // Obtain function entry points to WebGL 1 extension related functions.
      if (extString == 'ANGLE_instanced_arrays') webgl_enable_ANGLE_instanced_arrays(GLctx);
      if (extString == 'OES_vertex_array_object') webgl_enable_OES_vertex_array_object(GLctx);
      if (extString == 'WEBGL_draw_buffers') webgl_enable_WEBGL_draw_buffers(GLctx);
  
      if (extString == 'WEBGL_draw_instanced_base_vertex_base_instance') webgl_enable_WEBGL_draw_instanced_base_vertex_base_instance(GLctx);
      if (extString == 'WEBGL_multi_draw_instanced_base_vertex_base_instance') webgl_enable_WEBGL_multi_draw_instanced_base_vertex_base_instance(GLctx);
  
      if (extString == 'WEBGL_multi_draw') webgl_enable_WEBGL_multi_draw(GLctx);
      if (extString == 'EXT_polygon_offset_clamp') webgl_enable_EXT_polygon_offset_clamp(GLctx);
      if (extString == 'EXT_clip_control') webgl_enable_EXT_clip_control(GLctx);
      if (extString == 'WEBGL_polygon_mode') webgl_enable_WEBGL_polygon_mode(GLctx);
  
      var ext = context.GLctx.getExtension(extString);
      return !!ext;
    };

  
  var _emscripten_webgl_do_get_current_context = () => GL.currentContext ? GL.currentContext.handle : 0;
  var _emscripten_webgl_get_current_context = _emscripten_webgl_do_get_current_context;

  var ENV = {
  };
  
  var getExecutableName = () => thisProgram;
  var getEnvStrings = () => {
      if (!getEnvStrings.strings) {
        // Default values.
        var lang = (globalThis.navigator?.language ?? 'C').replace('-', '_') + '.UTF-8';
        var env = {
          'USER': 'web_user',
          'LOGNAME': 'web_user',
          'PATH': '/',
          'PWD': '/',
          'HOME': '/home/web_user',
          'LANG': lang,
          '_': getExecutableName()
        };
        // Apply the user-provided values, if any.
        for (var x in ENV) {
          // x is a key in ENV; if ENV[x] is undefined, that means it was
          // explicitly set to be so. We allow user code to do that to
          // force variables with default values to remain unset.
          if (ENV[x] === undefined) delete env[x];
          else env[x] = ENV[x];
        }
        var strings = [];
        for (var x in env) {
          strings.push(`${x}=${env[x]}`);
        }
        getEnvStrings.strings = strings;
      }
      return getEnvStrings.strings;
    };
  
  
  var _environ_get = (__environ, environ_buf) => {
      var bufSize = 0;
      var envp = 0;
      for (var string of getEnvStrings()) {
        var ptr = environ_buf + bufSize;
        HEAPU32[(((__environ)+(envp))>>2)] = ptr;
        bufSize += stringToUTF8(string, ptr, Infinity) + 1;
        envp += 4;
      }
      return 0;
    };

  
  
  var _environ_sizes_get = (penviron_count, penviron_buf_size) => {
      var strings = getEnvStrings();
      HEAPU32[((penviron_count)>>2)] = strings.length;
      var bufSize = 0;
      for (var string of strings) {
        bufSize += lengthBytesUTF8(string) + 1;
      }
      HEAPU32[((penviron_buf_size)>>2)] = bufSize;
      return 0;
    };


  function _fd_close(fd) {
  try {
  
      var stream = SYSCALLS.getStreamFromFD(fd);
      FS.close(stream);
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return e.errno;
  }
  }
  

  
  /** @param {number=} offset */
  var doReadv = (stream, iov, iovcnt, offset) => {
      var ret = 0;
      for (var i = 0; i < iovcnt; i++) {
        var ptr = HEAPU32[((iov)>>2)];
        var len = HEAPU32[(((iov)+(4))>>2)];
        iov += 8;
        try {
          var curr = FS.read(stream, HEAP8, ptr, len, offset);
        } catch (e) {
          // On a non-blocking stream a subsequent read may would-block after we
          // already gathered data. POSIX readv is a single gather-read: return
          // what we have rather than failing the whole call.
          if (ret > 0 && e instanceof FS.ErrnoError &&
              (e.errno == 6 || e.errno == 6)) {
            break;
          }
          throw e;
        }
        if (curr < 0) return -1;
        ret += curr;
        if (curr < len) break; // nothing more to read
        if (typeof offset != 'undefined') {
          offset += curr;
        }
      }
      return ret;
    };
  
  
  function _fd_read(fd, iov, iovcnt, pnum) {
  try {
  
      var stream = SYSCALLS.getStreamFromFD(fd);
      var num = doReadv(stream, iov, iovcnt);
      HEAPU32[((pnum)>>2)] = num;
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return e.errno;
  }
  }
  

  
  
  function _fd_seek(fd, offset, whence, newOffset) {
    offset = bigintToI53Checked(offset);
  
  
  try {
  
      if (isNaN(offset)) return 22;
      var stream = SYSCALLS.getStreamFromFD(fd);
      FS.llseek(stream, offset, whence);
      HEAP64[((newOffset)>>3)] = BigInt(stream.position);
      if (stream.getdents && !offset && whence === 0) stream.getdents = null; // reset readdir state
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return e.errno;
  }
  ;
  }

  
  
  /** @param {number=} offset */
  var doWritev = (stream, iov, iovcnt, offset) => {
      // Gather all iovecs into one contiguous buffer and issue a single
      // FS.write, matching POSIX writev's single gather-write semantics (as
      // __syscall_sendmsg already does). Per-iovec writes fragment a stream
      // socket send into multiple segments, breaking stream byte semantics.
      if (iovcnt == 1) {
        // Single iovec: write directly from HEAP8, no gather buffer needed.
        return FS.write(stream, HEAP8, HEAPU32[((iov)>>2)], HEAPU32[(((iov)+(4))>>2)], offset);
      }
      var total = 0;
      for (var i = 0, p = iov; i < iovcnt; i++, p += 8) {
        total += HEAPU32[(((p)+(4))>>2)];
      }
      var view = new Uint8Array(total);
      var voff = 0;
      for (var i = 0; i < iovcnt; i++, iov += 8) {
        var ptr = HEAPU32[((iov)>>2)];
        var len = HEAPU32[(((iov)+(4))>>2)];
        view.set(HEAPU8.subarray(ptr, ptr + len), voff);
        voff += len;
      }
      return FS.write(stream, view, 0, total, offset);
    };
  
  
  function _fd_write(fd, iov, iovcnt, pnum) {
  try {
  
      var stream = SYSCALLS.getStreamFromFD(fd);
      var num = doWritev(stream, iov, iovcnt);
      HEAPU32[((pnum)>>2)] = num;
      return 0;
    } catch (e) {
    if (typeof FS == 'undefined' || !(e.name === 'ErrnoError')) throw e;
    return e.errno;
  }
  }
  

  
  
  
  function getFullscreenElement() {
      return document.fullscreenElement
             ?? document.webkitFullscreenElement
             ;
    }
  
  /** @param {number=} timeout */
  var safeSetTimeout = (func, timeout) => {
      
      return setTimeout(() => {
        
        callUserCallback(func);
      }, timeout);
    };
  
  var warnOnce = (text) => {
      warnOnce.shown ||= {};
      if (!warnOnce.shown[text]) {
        warnOnce.shown[text] = 1;
        if (ENVIRONMENT_IS_NODE) text = 'warning: ' + text;
        err(text);
      }
    };
  
  
  
  
  var Browser = {
  useWebGL:false,
  isFullscreen:false,
  pointerLock:false,
  moduleContextCreatedCallbacks:[],
  preloadedImages:{
  },
  preloadedAudios:{
  },
  getCanvas:() => Module['canvas'],
  init() {
        if (Browser.initted) return;
        Browser.initted = true;
  
        // Support for plugins that can process preloaded files. You can add more of these to
        // your app by creating and appending to preloadPlugins.
        //
        // Each plugin is asked if it can handle a file based on the file's name. If it can,
        // it is given the file's raw data. When it is done, it calls a callback with the file's
        // (possibly modified) data. For example, a plugin might decompress a file, or it
        // might create some side data structure for use later (like an Image element, etc.).
  
        var imagePlugin = {};
        imagePlugin['canHandle'] = (name) => {
          return !Module['noImageDecoding'] && /\.(jpg|jpeg|png|bmp|webp)$/i.test(name);
        };
        imagePlugin['handle'] = async (byteArray, name) => {
          var b = new Blob([byteArray], { type: Browser.getMimetype(name) });
          if (b.size !== byteArray.length) { // Safari bug #118630
            // Safari's Blob can only take an ArrayBuffer
            b = new Blob([(new Uint8Array(byteArray)).buffer], { type: Browser.getMimetype(name) });
          }
          var url = URL.createObjectURL(b);
          return new Promise((resolve, reject) => {
            var img = new Image();
            img.onload = () => {
              var canvas = /** @type {!HTMLCanvasElement} */ (document.createElement('canvas'));
              canvas.width = img.width;
              canvas.height = img.height;
              var ctx = canvas.getContext('2d');
              ctx.drawImage(img, 0, 0);
              Browser.preloadedImages[name] = canvas;
              URL.revokeObjectURL(url);
              resolve(byteArray);
            };
            img.onerror = (event) => {
              err(`Image ${url} could not be decoded`);
              reject();
            };
            img.src = url;
          });
        };
        preloadPlugins.push(imagePlugin);
  
        var audioPlugin = {};
        audioPlugin['canHandle'] = (name) => {
          return !Module['noAudioDecoding'] && name.slice(-4) in { '.ogg': 1, '.wav': 1, '.mp3': 1 };
        };
        audioPlugin['handle'] = async (byteArray, name) => {
          return new Promise((resolve, reject) => {
            var done = false;
            function finish(audio) {
              if (done) return;
              done = true;
              Browser.preloadedAudios[name] = audio;
              resolve(byteArray);
            }
            var b = new Blob([byteArray], { type: Browser.getMimetype(name) });
            var url = URL.createObjectURL(b); // XXX we never revoke this!
            var audio = new Audio();
            audio.addEventListener('canplaythrough', () => finish(audio)); // use addEventListener due to chromium bug 124926
            audio.onerror = (event) => {
              if (done) return;
              err(`warning: browser could not fully decode audio ${name}, trying slower base64 approach`);
              function encode64(data) {
                var BASE = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
                var PAD = '=';
                var ret = '';
                var leftchar = 0;
                var leftbits = 0;
                for (var byte of data) {
                  leftchar = (leftchar << 8) | byte;
                  leftbits += 8;
                  while (leftbits >= 6) {
                    var curr = (leftchar >> (leftbits-6)) & 0x3f;
                    leftbits -= 6;
                    ret += BASE[curr];
                  }
                }
                if (leftbits == 2) {
                  ret += BASE[(leftchar&3) << 4];
                  ret += PAD + PAD;
                } else if (leftbits == 4) {
                  ret += BASE[(leftchar&0xf) << 2];
                  ret += PAD;
                }
                return ret;
              }
              audio.src = 'data:audio/x-' + name.slice(-3) + ';base64,' + encode64(byteArray);
              finish(audio); // we don't wait for confirmation this worked - but it's worth trying
            };
            audio.src = url;
            // workaround for chrome bug 124926 - we do not always get oncanplaythrough or onerror
            safeSetTimeout(() => {
              finish(audio); // try to use it even though it is not necessarily ready to play
            }, 10000);
          });
        };
        preloadPlugins.push(audioPlugin);
  
        // Canvas event setup
  
        function pointerLockChange() {
          var canvas = Browser.getCanvas();
          Browser.pointerLock = document.pointerLockElement === canvas;
        }
        var canvas = Browser.getCanvas();
        if (canvas) {
          // forced aspect ratio can be enabled by defining 'forcedAspectRatio' on Module
          // Module['forcedAspectRatio'] = 4 / 3;
  
          document.addEventListener('pointerlockchange', pointerLockChange);
  
          if (Module['elementPointerLock']) {
            canvas.addEventListener('click', (ev) => {
              if (!Browser.pointerLock && Browser.getCanvas().requestPointerLock) {
                Browser.getCanvas().requestPointerLock();
                ev.preventDefault();
              }
            });
          }
        }
      },
  createContext(/** @type {HTMLCanvasElement} */ canvas, useWebGL, setInModule, webGLContextAttributes) {
        if (useWebGL && Module['ctx'] && canvas == Browser.getCanvas()) return Module['ctx']; // no need to recreate GL context if it's already been created for this canvas.
  
        var ctx;
        var contextHandle;
        if (useWebGL) {
          // For GLES2/desktop GL compatibility, adjust a few defaults to be different to WebGL defaults, so that they align better with the desktop defaults.
          var contextAttributes = {
            antialias: false,
            alpha: false,
            majorVersion: (typeof WebGL2RenderingContext != 'undefined') ? 2 : 1,
          };
  
          if (webGLContextAttributes) {
            for (var attribute in webGLContextAttributes) {
              contextAttributes[attribute] = webGLContextAttributes[attribute];
            }
          }
  
          // This check of existence of GL is here to satisfy Closure compiler, which yells if variable GL is referenced below but GL object is not
          // actually compiled in because application is not doing any GL operations. TODO: Ideally if GL is not being used, this function
          // Browser.createContext() should not even be emitted.
          if (typeof GL != 'undefined') {
            contextHandle = GL.createContext(canvas, contextAttributes);
            if (contextHandle) {
              ctx = GL.getContext(contextHandle).GLctx;
            }
          }
        } else {
          ctx = canvas.getContext('2d');
        }
  
        if (!ctx) return null;
  
        if (setInModule) {
          Module['ctx'] = ctx;
          if (useWebGL) GL.makeContextCurrent(contextHandle);
          Browser.useWebGL = useWebGL;
          Browser.moduleContextCreatedCallbacks.forEach((callback) => callback());
          Browser.init();
        }
        return ctx;
      },
  fullscreenHandlersInstalled:false,
  lockPointer:undefined,
  resizeCanvas:undefined,
  requestFullscreen(lockPointer, resizeCanvas) {
        Browser.lockPointer = lockPointer;
        Browser.resizeCanvas = resizeCanvas;
        if (typeof Browser.lockPointer == 'undefined') Browser.lockPointer = true;
        if (typeof Browser.resizeCanvas == 'undefined') Browser.resizeCanvas = false;
  
        var canvas = Browser.getCanvas();
        function fullscreenChange() {
          Browser.isFullscreen = false;
          var canvasContainer = canvas.parentNode;
          if (getFullscreenElement() === canvasContainer) {
            canvas.exitFullscreen = Browser.exitFullscreen;
            if (Browser.lockPointer) canvas.requestPointerLock();
            Browser.isFullscreen = true;
            if (Browser.resizeCanvas) {
              Browser.setFullscreenCanvasSize();
            } else {
              Browser.updateCanvasDimensions(canvas);
            }
          } else {
            // remove the full screen specific parent of the canvas again to restore the HTML structure from before going full screen
            canvasContainer.parentNode.insertBefore(canvas, canvasContainer);
            canvasContainer.parentNode.removeChild(canvasContainer);
  
            if (Browser.resizeCanvas) {
              Browser.setWindowedCanvasSize();
            } else {
              Browser.updateCanvasDimensions(canvas);
            }
          }
        }
  
        if (!Browser.fullscreenHandlersInstalled) {
          Browser.fullscreenHandlersInstalled = true;
          document.addEventListener('fullscreenchange', fullscreenChange);
          document.addEventListener('webkitfullscreenchange', fullscreenChange);
        }
  
        // create a new parent to ensure the canvas has no siblings. this allows browsers to optimize full screen performance when its parent is the full screen root
        var canvasContainer = document.createElement('div');
        canvas.parentNode.insertBefore(canvasContainer, canvas);
        canvasContainer.appendChild(canvas);
  
        // use parent of canvas as full screen root to allow aspect ratio correction (Firefox stretches the root to screen size)
        // Safari didn't support Element.requestFullscreen until 16.4
        // See: https://developer.mozilla.org/en-US/docs/Web/API/Element/requestFullscreen
        /** @suppress {checkTypes} */
        canvasContainer.requestFullscreen ??= (canvasContainer['webkitRequestFullscreen'] ? () => canvasContainer['webkitRequestFullscreen'](Element.ALLOW_KEYBOARD_INPUT) : null) ??
                                              (canvasContainer['webkitRequestFullScreen'] ? () => canvasContainer['webkitRequestFullScreen'](Element.ALLOW_KEYBOARD_INPUT) : null);
  
        canvasContainer.requestFullscreen();
      },
  exitFullscreen() {
        // This is workaround for chrome. Trying to exit from fullscreen
        // not in fullscreen state will cause 'TypeError: Document not active'
        // in chrome. See https://github.com/emscripten-core/emscripten/pull/8236
        if (!Browser.isFullscreen) {
          return false;
        }
  
        var CFS = document.exitFullscreen ?? document['webkitCancelFullScreen'];
        CFS.apply(document, []);
        return true;
      },
  safeSetTimeout(func, timeout) {
        // Legacy function, this is used by the SDL2 port so we need to keep it
        // around at least until that is updated.
        // See https://github.com/libsdl-org/SDL/pull/6304
        return safeSetTimeout(func, timeout);
      },
  getMimetype(name) {
        return {
          'jpg': 'image/jpeg',
          'jpeg': 'image/jpeg',
          'png': 'image/png',
          'bmp': 'image/bmp',
          'ogg': 'audio/ogg',
          'wav': 'audio/wav',
          'mp3': 'audio/mpeg'
        }[name.slice(name.lastIndexOf('.')+1)];
      },
  getUserMedia(func) {
        return navigator.mediaDevices.getUserMedia(func);
      },
  getMouseWheelDelta(event) {
        var delta = 0;
        switch (event.type) {
          case 'DOMMouseScroll':
            // 3 lines make up a step
            delta = event.detail / 3;
            break;
          case 'mousewheel':
            // 120 units make up a step
            delta = event.wheelDelta / 120;
            break;
          case 'wheel':
            delta = event.deltaY
            switch (event.deltaMode) {
              case 0:
                // DOM_DELTA_PIXEL: 100 pixels make up a step
                delta /= 100;
                break;
              case 1:
                // DOM_DELTA_LINE: 3 lines make up a step
                delta /= 3;
                break;
              case 2:
                // DOM_DELTA_PAGE: A page makes up 80 steps
                delta *= 80;
                break;
              default:
                abort('unrecognized mouse wheel delta mode: ' + event.deltaMode);
            }
            break;
          default:
            abort('unrecognized mouse wheel event: ' + event.type);
        }
        return delta;
      },
  mouseX:0,
  mouseY:0,
  mouseMovementX:0,
  mouseMovementY:0,
  touches:{
  },
  lastTouches:{
  },
  calculateMouseCoords(pageX, pageY) {
        // Calculate the movement based on the changes
        // in the coordinates.
        var canvas = Browser.getCanvas();
        var rect = canvas.getBoundingClientRect();
  
        var adjustedX = pageX - (window.scrollX + rect.left);
        var adjustedY = pageY - (window.scrollY + rect.top);
  
        // the canvas might be CSS-scaled compared to its backbuffer;
        // SDL-using content will want mouse coordinates in terms
        // of backbuffer units.
        adjustedX = adjustedX * (canvas.width / rect.width);
        adjustedY = adjustedY * (canvas.height / rect.height);
  
        return { x: adjustedX, y: adjustedY };
      },
  setMouseCoords(pageX, pageY) {
        const {x, y} = Browser.calculateMouseCoords(pageX, pageY);
        Browser.mouseMovementX = x - Browser.mouseX;
        Browser.mouseMovementY = y - Browser.mouseY;
        Browser.mouseX = x;
        Browser.mouseY = y;
      },
  calculateMouseEvent(event) { // event should be mousemove, mousedown or mouseup
        if (Browser.pointerLock) {
          // When the pointer is locked, calculate the coordinates
          // based on the movement of the mouse.
          Browser.mouseMovementX = event.movementX;
          Browser.mouseMovementY = event.movementY;
  
          // add the mouse delta to the current absolute mouse position
          Browser.mouseX += Browser.mouseMovementX;
          Browser.mouseY += Browser.mouseMovementY;
        } else {
          if (event.type === 'touchstart' || event.type === 'touchend' || event.type === 'touchmove') {
            var touch = event.touch;
            if (touch === undefined) {
              return; // the 'touch' property is only defined in SDL
  
            }
            var coords = Browser.calculateMouseCoords(touch.pageX, touch.pageY);
  
            if (event.type === 'touchstart') {
              Browser.lastTouches[touch.identifier] = coords;
              Browser.touches[touch.identifier] = coords;
            } else if (event.type === 'touchend' || event.type === 'touchmove') {
              var last = Browser.touches[touch.identifier];
              last ||= coords;
              Browser.lastTouches[touch.identifier] = last;
              Browser.touches[touch.identifier] = coords;
            }
            return;
          }
  
          Browser.setMouseCoords(event.pageX, event.pageY);
        }
      },
  resizeListeners:[],
  updateResizeListeners() {
        var canvas = Browser.getCanvas();
        Browser.resizeListeners.forEach((listener) => listener(canvas.width, canvas.height));
      },
  setCanvasSize(width, height, noUpdates) {
        var canvas = Browser.getCanvas();
        Browser.updateCanvasDimensions(canvas, width, height);
        if (!noUpdates) Browser.updateResizeListeners();
      },
  windowedWidth:0,
  windowedHeight:0,
  setFullscreenCanvasSize() {
        // check if SDL is available
        if (typeof SDL != 'undefined') {
          var flags = HEAPU32[((SDL.screen)>>2)];
          flags = flags | 0x00800000; // set SDL_FULLSCREEN flag
          HEAP32[((SDL.screen)>>2)] = flags;
        }
        Browser.updateCanvasDimensions(Browser.getCanvas());
        Browser.updateResizeListeners();
      },
  setWindowedCanvasSize() {
        // check if SDL is available
        if (typeof SDL != 'undefined') {
          var flags = HEAPU32[((SDL.screen)>>2)];
          flags = flags & ~0x00800000; // clear SDL_FULLSCREEN flag
          HEAP32[((SDL.screen)>>2)] = flags;
        }
        Browser.updateCanvasDimensions(Browser.getCanvas());
        Browser.updateResizeListeners();
      },
  updateCanvasDimensions(canvas, wNative, hNative) {
        if (wNative && hNative) {
          canvas.widthNative = wNative;
          canvas.heightNative = hNative;
        } else {
          wNative = canvas.widthNative;
          hNative = canvas.heightNative;
        }
        var w = wNative;
        var h = hNative;
        if ((getFullscreenElement() === canvas.parentNode) && (typeof screen != 'undefined')) {
           var factor = Math.min(screen.width / w, screen.height / h);
           w = Math.round(w * factor);
           h = Math.round(h * factor);
        }
        if (Browser.resizeCanvas) {
          if (canvas.width  != w) canvas.width  = w;
          if (canvas.height != h) canvas.height = h;
          if (typeof canvas.style != 'undefined') {
            canvas.style.removeProperty( 'width');
            canvas.style.removeProperty('height');
          }
        } else {
          if (canvas.width  != wNative) canvas.width  = wNative;
          if (canvas.height != hNative) canvas.height = hNative;
          if (typeof canvas.style != 'undefined') {
            if (w != wNative || h != hNative) {
              canvas.style.setProperty( 'width', w + 'px', 'important');
              canvas.style.setProperty('height', h + 'px', 'important');
            } else {
              canvas.style.removeProperty( 'width');
              canvas.style.removeProperty('height');
            }
          }
        }
      },
  };
  
  /** @constructor */
  function GLFW_Window(id, width, height, framebufferWidth, framebufferHeight, title, monitor, share) {
        this.id = id;
        this.x = 0;
        this.y = 0;
        this.fullscreen = false; // Used to determine if app is in fullscreen mode
        this.storedX = 0; // Used to store X before fullscreen
        this.storedY = 0; // Used to store Y before fullscreen
        this.width = width;
        this.height = height;
        this.framebufferWidth = framebufferWidth;
        this.framebufferHeight = framebufferHeight;
        this.storedWidth = width; // Used to store width before fullscreen
        this.storedHeight = height; // Used to store height before fullscreen
        this.title = title;
        this.monitor = monitor;
        this.share = share;
        this.attributes = {...GLFW.hints};
        this.inputModes = {
          0x00033001:0x00034001, // GLFW_CURSOR (GLFW_CURSOR_NORMAL)
          0x00033002:0, // GLFW_STICKY_KEYS
          0x00033003:0, // GLFW_STICKY_MOUSE_BUTTONS
        };
        this.buttons = 0;
        this.keys = new Array();
        this.domKeys = new Array();
        this.shouldClose = 0;
        this.title = null;
        this.windowPosFunc = 0; // GLFWwindowposfun
        this.windowSizeFunc = 0; // GLFWwindowsizefun
        this.windowCloseFunc = 0; // GLFWwindowclosefun
        this.windowRefreshFunc = 0; // GLFWwindowrefreshfun
        this.windowFocusFunc = 0; // GLFWwindowfocusfun
        this.windowIconifyFunc = 0; // GLFWwindowiconifyfun
        this.windowMaximizeFunc = 0; // GLFWwindowmaximizefun
        this.framebufferSizeFunc = 0; // GLFWframebuffersizefun
        this.windowContentScaleFunc = 0; // GLFWwindowcontentscalefun
        this.mouseButtonFunc = 0; // GLFWmousebuttonfun
        this.cursorPosFunc = 0; // GLFWcursorposfun
        this.cursorEnterFunc = 0; // GLFWcursorenterfun
        this.scrollFunc = 0; // GLFWscrollfun
        this.dropFunc = 0; // GLFWdropfun
        this.keyFunc = 0; // GLFWkeyfun
        this.charFunc = 0; // GLFWcharfun
        this.userptr = 0;
      }
  
  
  
  
  
  
  
  var _emscripten_set_window_title = (title) => document.title = UTF8ToString(title);
  
  
  
  
  
  
  
  
  var GLFW = {
  WindowFromId:(id) => {
        if (id <= 0 || !GLFW.windows) return null;
        return GLFW.windows[id - 1];
      },
  joystickFunc:0,
  errorFunc:0,
  monitorFunc:0,
  active:null,
  scale:null,
  windows:null,
  monitors:null,
  monitorString:null,
  versionString:null,
  initialTime:null,
  extensions:null,
  devicePixelRatioMQL:null,
  hints:null,
  primaryTouchId:null,
  defaultHints:{
  131073:0,
  131074:0,
  131075:1,
  131076:1,
  131077:1,
  131082:0,
  135169:8,
  135170:8,
  135171:8,
  135172:8,
  135173:24,
  135174:8,
  135175:0,
  135176:0,
  135177:0,
  135178:0,
  135179:0,
  135180:0,
  135181:0,
  135182:0,
  135183:0,
  139265:196609,
  139266:1,
  139267:0,
  139268:0,
  139269:0,
  139270:0,
  139271:0,
  139272:0,
  139276:0,
  },
  DOMToGLFWKeyCode:(keycode) => {
        switch (keycode) {
          // these keycodes are only defined for GLFW3, assume they are the same for GLFW2
          case 0x20:return 32; // DOM_VK_SPACE -> GLFW_KEY_SPACE
          case 0xDE:return 39; // DOM_VK_QUOTE -> GLFW_KEY_APOSTROPHE
          case 0xBC:return 44; // DOM_VK_COMMA -> GLFW_KEY_COMMA
          case 0xAD:return 45; // DOM_VK_HYPHEN_MINUS -> GLFW_KEY_MINUS
          case 0xBD:return 45; // DOM_VK_MINUS -> GLFW_KEY_MINUS
          case 0xBE:return 46; // DOM_VK_PERIOD -> GLFW_KEY_PERIOD
          case 0xBF:return 47; // DOM_VK_SLASH -> GLFW_KEY_SLASH
          case 0x30:return 48; // DOM_VK_0 -> GLFW_KEY_0
          case 0x31:return 49; // DOM_VK_1 -> GLFW_KEY_1
          case 0x32:return 50; // DOM_VK_2 -> GLFW_KEY_2
          case 0x33:return 51; // DOM_VK_3 -> GLFW_KEY_3
          case 0x34:return 52; // DOM_VK_4 -> GLFW_KEY_4
          case 0x35:return 53; // DOM_VK_5 -> GLFW_KEY_5
          case 0x36:return 54; // DOM_VK_6 -> GLFW_KEY_6
          case 0x37:return 55; // DOM_VK_7 -> GLFW_KEY_7
          case 0x38:return 56; // DOM_VK_8 -> GLFW_KEY_8
          case 0x39:return 57; // DOM_VK_9 -> GLFW_KEY_9
          case 0x3B:return 59; // DOM_VK_SEMICOLON -> GLFW_KEY_SEMICOLON
          case 0x3D:return 61; // DOM_VK_EQUALS -> GLFW_KEY_EQUAL
          case 0xBB:return 61; // DOM_VK_EQUALS -> GLFW_KEY_EQUAL
          case 0x41:return 65; // DOM_VK_A -> GLFW_KEY_A
          case 0x42:return 66; // DOM_VK_B -> GLFW_KEY_B
          case 0x43:return 67; // DOM_VK_C -> GLFW_KEY_C
          case 0x44:return 68; // DOM_VK_D -> GLFW_KEY_D
          case 0x45:return 69; // DOM_VK_E -> GLFW_KEY_E
          case 0x46:return 70; // DOM_VK_F -> GLFW_KEY_F
          case 0x47:return 71; // DOM_VK_G -> GLFW_KEY_G
          case 0x48:return 72; // DOM_VK_H -> GLFW_KEY_H
          case 0x49:return 73; // DOM_VK_I -> GLFW_KEY_I
          case 0x4A:return 74; // DOM_VK_J -> GLFW_KEY_J
          case 0x4B:return 75; // DOM_VK_K -> GLFW_KEY_K
          case 0x4C:return 76; // DOM_VK_L -> GLFW_KEY_L
          case 0x4D:return 77; // DOM_VK_M -> GLFW_KEY_M
          case 0x4E:return 78; // DOM_VK_N -> GLFW_KEY_N
          case 0x4F:return 79; // DOM_VK_O -> GLFW_KEY_O
          case 0x50:return 80; // DOM_VK_P -> GLFW_KEY_P
          case 0x51:return 81; // DOM_VK_Q -> GLFW_KEY_Q
          case 0x52:return 82; // DOM_VK_R -> GLFW_KEY_R
          case 0x53:return 83; // DOM_VK_S -> GLFW_KEY_S
          case 0x54:return 84; // DOM_VK_T -> GLFW_KEY_T
          case 0x55:return 85; // DOM_VK_U -> GLFW_KEY_U
          case 0x56:return 86; // DOM_VK_V -> GLFW_KEY_V
          case 0x57:return 87; // DOM_VK_W -> GLFW_KEY_W
          case 0x58:return 88; // DOM_VK_X -> GLFW_KEY_X
          case 0x59:return 89; // DOM_VK_Y -> GLFW_KEY_Y
          case 0x5a:return 90; // DOM_VK_Z -> GLFW_KEY_Z
          case 0xDB:return 91; // DOM_VK_OPEN_BRACKET -> GLFW_KEY_LEFT_BRACKET
          case 0xDC:return 92; // DOM_VK_BACKSLASH -> GLFW_KEY_BACKSLASH
          case 0xDD:return 93; // DOM_VK_CLOSE_BRACKET -> GLFW_KEY_RIGHT_BRACKET
          case 0xC0:return 96; // DOM_VK_BACK_QUOTE -> GLFW_KEY_GRAVE_ACCENT
  
          case 0x1B:return 256; // DOM_VK_ESCAPE -> GLFW_KEY_ESCAPE
          case 0x0D:return 257; // DOM_VK_RETURN -> GLFW_KEY_ENTER
          case 0x09:return 258; // DOM_VK_TAB -> GLFW_KEY_TAB
          case 0x08:return 259; // DOM_VK_BACK -> GLFW_KEY_BACKSPACE
          case 0x2D:return 260; // DOM_VK_INSERT -> GLFW_KEY_INSERT
          case 0x2E:return 261; // DOM_VK_DELETE -> GLFW_KEY_DELETE
          case 0x27:return 262; // DOM_VK_RIGHT -> GLFW_KEY_RIGHT
          case 0x25:return 263; // DOM_VK_LEFT -> GLFW_KEY_LEFT
          case 0x28:return 264; // DOM_VK_DOWN -> GLFW_KEY_DOWN
          case 0x26:return 265; // DOM_VK_UP -> GLFW_KEY_UP
          case 0x21:return 266; // DOM_VK_PAGE_UP -> GLFW_KEY_PAGE_UP
          case 0x22:return 267; // DOM_VK_PAGE_DOWN -> GLFW_KEY_PAGE_DOWN
          case 0x24:return 268; // DOM_VK_HOME -> GLFW_KEY_HOME
          case 0x23:return 269; // DOM_VK_END -> GLFW_KEY_END
          case 0x14:return 280; // DOM_VK_CAPS_LOCK -> GLFW_KEY_CAPS_LOCK
          case 0x91:return 281; // DOM_VK_SCROLL_LOCK -> GLFW_KEY_SCROLL_LOCK
          case 0x90:return 282; // DOM_VK_NUM_LOCK -> GLFW_KEY_NUM_LOCK
          case 0x2C:return 283; // DOM_VK_SNAPSHOT -> GLFW_KEY_PRINT_SCREEN
          case 0x13:return 284; // DOM_VK_PAUSE -> GLFW_KEY_PAUSE
          case 0x70:return 290; // DOM_VK_F1 -> GLFW_KEY_F1
          case 0x71:return 291; // DOM_VK_F2 -> GLFW_KEY_F2
          case 0x72:return 292; // DOM_VK_F3 -> GLFW_KEY_F3
          case 0x73:return 293; // DOM_VK_F4 -> GLFW_KEY_F4
          case 0x74:return 294; // DOM_VK_F5 -> GLFW_KEY_F5
          case 0x75:return 295; // DOM_VK_F6 -> GLFW_KEY_F6
          case 0x76:return 296; // DOM_VK_F7 -> GLFW_KEY_F7
          case 0x77:return 297; // DOM_VK_F8 -> GLFW_KEY_F8
          case 0x78:return 298; // DOM_VK_F9 -> GLFW_KEY_F9
          case 0x79:return 299; // DOM_VK_F10 -> GLFW_KEY_F10
          case 0x7A:return 300; // DOM_VK_F11 -> GLFW_KEY_F11
          case 0x7B:return 301; // DOM_VK_F12 -> GLFW_KEY_F12
          case 0x7C:return 302; // DOM_VK_F13 -> GLFW_KEY_F13
          case 0x7D:return 303; // DOM_VK_F14 -> GLFW_KEY_F14
          case 0x7E:return 304; // DOM_VK_F15 -> GLFW_KEY_F15
          case 0x7F:return 305; // DOM_VK_F16 -> GLFW_KEY_F16
          case 0x80:return 306; // DOM_VK_F17 -> GLFW_KEY_F17
          case 0x81:return 307; // DOM_VK_F18 -> GLFW_KEY_F18
          case 0x82:return 308; // DOM_VK_F19 -> GLFW_KEY_F19
          case 0x83:return 309; // DOM_VK_F20 -> GLFW_KEY_F20
          case 0x84:return 310; // DOM_VK_F21 -> GLFW_KEY_F21
          case 0x85:return 311; // DOM_VK_F22 -> GLFW_KEY_F22
          case 0x86:return 312; // DOM_VK_F23 -> GLFW_KEY_F23
          case 0x87:return 313; // DOM_VK_F24 -> GLFW_KEY_F24
          case 0x88:return 314; // 0x88 (not used?) -> GLFW_KEY_F25
          case 0x60:return 320; // DOM_VK_NUMPAD0 -> GLFW_KEY_KP_0
          case 0x61:return 321; // DOM_VK_NUMPAD1 -> GLFW_KEY_KP_1
          case 0x62:return 322; // DOM_VK_NUMPAD2 -> GLFW_KEY_KP_2
          case 0x63:return 323; // DOM_VK_NUMPAD3 -> GLFW_KEY_KP_3
          case 0x64:return 324; // DOM_VK_NUMPAD4 -> GLFW_KEY_KP_4
          case 0x65:return 325; // DOM_VK_NUMPAD5 -> GLFW_KEY_KP_5
          case 0x66:return 326; // DOM_VK_NUMPAD6 -> GLFW_KEY_KP_6
          case 0x67:return 327; // DOM_VK_NUMPAD7 -> GLFW_KEY_KP_7
          case 0x68:return 328; // DOM_VK_NUMPAD8 -> GLFW_KEY_KP_8
          case 0x69:return 329; // DOM_VK_NUMPAD9 -> GLFW_KEY_KP_9
          case 0x6E:return 330; // DOM_VK_DECIMAL -> GLFW_KEY_KP_DECIMAL
          case 0x6F:return 331; // DOM_VK_DIVIDE -> GLFW_KEY_KP_DIVIDE
          case 0x6A:return 332; // DOM_VK_MULTIPLY -> GLFW_KEY_KP_MULTIPLY
          case 0x6D:return 333; // DOM_VK_SUBTRACT -> GLFW_KEY_KP_SUBTRACT
          case 0x6B:return 334; // DOM_VK_ADD -> GLFW_KEY_KP_ADD
          // case 0x0D:return 335; // DOM_VK_RETURN -> GLFW_KEY_KP_ENTER (DOM_KEY_LOCATION_RIGHT)
          // case 0x61:return 336; // DOM_VK_EQUALS -> GLFW_KEY_KP_EQUAL (DOM_KEY_LOCATION_RIGHT)
          case 0x10:return 340; // DOM_VK_SHIFT -> GLFW_KEY_LEFT_SHIFT
          case 0x11:return 341; // DOM_VK_CONTROL -> GLFW_KEY_LEFT_CONTROL
          case 0x12:return 342; // DOM_VK_ALT -> GLFW_KEY_LEFT_ALT
          case 0x5B:return 343; // DOM_VK_WIN -> GLFW_KEY_LEFT_SUPER
          case 0xE0:return 343; // DOM_VK_META -> GLFW_KEY_LEFT_SUPER
          // case 0x10:return 344; // DOM_VK_SHIFT -> GLFW_KEY_RIGHT_SHIFT (DOM_KEY_LOCATION_RIGHT)
          // case 0x11:return 345; // DOM_VK_CONTROL -> GLFW_KEY_RIGHT_CONTROL (DOM_KEY_LOCATION_RIGHT)
          // case 0x12:return 346; // DOM_VK_ALT -> GLFW_KEY_RIGHT_ALT (DOM_KEY_LOCATION_RIGHT)
          // case 0x5B:return 347; // DOM_VK_WIN -> GLFW_KEY_RIGHT_SUPER (DOM_KEY_LOCATION_RIGHT)
          case 0x5D:return 348; // DOM_VK_CONTEXT_MENU -> GLFW_KEY_MENU
          // XXX: GLFW_KEY_WORLD_1, GLFW_KEY_WORLD_2 what are these?
          default:return -1; // GLFW_KEY_UNKNOWN
        };
      },
  getModBits:(win) => {
        var mod = 0;
        if (win.keys[340]) mod |= 0x0001; // GLFW_MOD_SHIFT
        if (win.keys[341]) mod |= 0x0002; // GLFW_MOD_CONTROL
        if (win.keys[342]) mod |= 0x0004; // GLFW_MOD_ALT
        if (win.keys[343] || win.keys[348]) mod |= 0x0008; // GLFW_MOD_SUPER
        // add caps and num lock keys? only if lock_key_mod is set
        return mod;
      },
  onKeyPress:(event) => {
        if (!GLFW.active || !GLFW.active.charFunc) return;
        if (event.ctrlKey || event.metaKey) return;
  
        // correct unicode charCode is only available with onKeyPress event
        var charCode = event.charCode;
        if (charCode == 0 || (charCode >= 0x00 && charCode <= 0x1F)) return;
  
        getWasmTableEntry(GLFW.active.charFunc)(GLFW.active.id, charCode);
      },
  onKeyChanged:(keyCode, status) => {
        if (!GLFW.active) return;
  
        var key = GLFW.DOMToGLFWKeyCode(keyCode);
        if (key == -1) return;
  
        var repeat = status && GLFW.active.keys[key];
        GLFW.active.keys[key] = status;
        GLFW.active.domKeys[keyCode] = status;
  
        if (GLFW.active.keyFunc) {
          if (repeat) status = 2; // GLFW_REPEAT
          getWasmTableEntry(GLFW.active.keyFunc)(GLFW.active.id, key, keyCode, status, GLFW.getModBits(GLFW.active));
        }
      },
  onGamepadConnected:(event) => {
        GLFW.refreshJoysticks();
      },
  onGamepadDisconnected:(event) => {
        GLFW.refreshJoysticks();
      },
  onKeydown:(event) => {
        GLFW.onKeyChanged(event.keyCode, 1); // GLFW_PRESS or GLFW_REPEAT
  
        // This logic comes directly from the sdl implementation. We cannot
        // call preventDefault on all keydown events otherwise onKeyPress will
        // not get called
        if (event.key == 'Backspace' || event.key == 'Tab') {
          event.preventDefault();
        }
      },
  onKeyup:(event) => {
        GLFW.onKeyChanged(event.keyCode, 0); // GLFW_RELEASE
      },
  onBlur:(event) => {
        if (!GLFW.active) return;
  
        for (var i = 0; i < GLFW.active.domKeys.length; ++i) {
          if (GLFW.active.domKeys[i]) {
            GLFW.onKeyChanged(i, 0); // GLFW_RELEASE
          }
        }
      },
  onMousemove:(event) => {
        if (!GLFW.active) return;
  
        if (event.type === 'touchmove') {
          // Handling for touch events that are being converted to mouse input.
  
          // Don't let the browser fire a duplicate mouse event.
          event.preventDefault();
  
          let primaryChanged = false;
          for (let i of event.changedTouches) {
            // If our chosen primary touch moved, update Browser mouse coords
            if (GLFW.primaryTouchId === i.identifier) {
              Browser.setMouseCoords(i.pageX, i.pageY);
              primaryChanged = true;
              break;
            }
          }
  
          if (!primaryChanged) {
            // Do not send mouse events if some touch other than the primary triggered this.
            return;
          }
  
        } else {
          // Handling for non-touch mouse input events.
          Browser.calculateMouseEvent(event);
        }
  
        if (event.target != Browser.getCanvas() || !GLFW.active.cursorPosFunc) return;
  
        if (GLFW.active.cursorPosFunc) {
          getWasmTableEntry(GLFW.active.cursorPosFunc)(GLFW.active.id, Browser.mouseX, Browser.mouseY);
        }
      },
  DOMToGLFWMouseButton:(event) => {
        // DOM and glfw have different button codes.
        // See http://www.w3schools.com/jsref/event_button.asp.
        var eventButton = event['button'];
        if (eventButton > 0) {
          if (eventButton == 1) {
            eventButton = 2;
          } else {
            eventButton = 1;
          }
        }
        return eventButton;
      },
  onMouseenter:(event) => {
        if (!GLFW.active) return;
  
        if (event.target != Browser.getCanvas()) return;
  
        if (GLFW.active.cursorEnterFunc) {
          getWasmTableEntry(GLFW.active.cursorEnterFunc)(GLFW.active.id, 1);
        }
      },
  onMouseleave:(event) => {
        if (!GLFW.active) return;
  
        if (event.target != Browser.getCanvas()) return;
  
        if (GLFW.active.cursorEnterFunc) {
          getWasmTableEntry(GLFW.active.cursorEnterFunc)(GLFW.active.id, 0);
        }
      },
  onMouseButtonChanged:(event, status) => {
        if (!GLFW.active) return;
  
        if (event.target != Browser.getCanvas()) return;
  
        // Is this from a touch event?
        const isTouchType = event.type === 'touchstart' || event.type === 'touchend' || event.type === 'touchcancel';
  
        // Only emulating mouse left-click behavior for touches.
        let eventButton = 0;
        if (isTouchType) {
          // Handling for touch events that are being converted to mouse input.
  
          // Don't let the browser fire a duplicate mouse event.
          event.preventDefault();
  
          let primaryChanged = false;
  
          // Set a primary touch if we have none.
          if (GLFW.primaryTouchId === null && event.type === 'touchstart' && event.targetTouches.length > 0) {
            // Pick the first touch that started in the canvas and treat it as primary.
            const chosenTouch = event.targetTouches[0];
            GLFW.primaryTouchId = chosenTouch.identifier;
  
            Browser.setMouseCoords(chosenTouch.pageX, chosenTouch.pageY);
            primaryChanged = true;
          } else if (event.type === 'touchend' || event.type === 'touchcancel') {
            // Clear the primary touch if it ended.
            for (let i of event.changedTouches) {
              // If our chosen primary touch ended, remove it.
              if (GLFW.primaryTouchId === i.identifier) {
                GLFW.primaryTouchId = null;
                primaryChanged = true;
                break;
              }
            }
          }
  
          if (!primaryChanged) {
            // Do not send mouse events if some touch other than the primary triggered this.
            return;
          }
  
        } else {
          // Handling for non-touch mouse input events.
          Browser.calculateMouseEvent(event);
          eventButton = GLFW.DOMToGLFWMouseButton(event);
        }
  
        if (status == 1) { // GLFW_PRESS
          GLFW.active.buttons |= (1 << eventButton);
          try {
            event.target.setCapture();
          } catch (e) {}
        } else {  // GLFW_RELEASE
          GLFW.active.buttons &= ~(1 << eventButton);
        }
  
        // Send mouse event to GLFW.
        if (GLFW.active.mouseButtonFunc) {
          getWasmTableEntry(GLFW.active.mouseButtonFunc)(GLFW.active.id, eventButton, status, GLFW.getModBits(GLFW.active));
        }
      },
  onMouseButtonDown:(event) => {
        if (!GLFW.active) return;
        GLFW.onMouseButtonChanged(event, 1); // GLFW_PRESS
      },
  onMouseButtonUp:(event) => {
        if (!GLFW.active) return;
        GLFW.onMouseButtonChanged(event, 0); // GLFW_RELEASE
      },
  onMouseWheel:(event) => {
        // Note the minus sign that flips browser wheel direction (positive direction scrolls page down) to native wheel direction (positive direction is mouse wheel up)
        var delta = -Browser.getMouseWheelDelta(event);
        delta = (delta == 0) ? 0 : (delta > 0 ? Math.max(delta, 1) : Math.min(delta, -1)); // Quantize to integer so that minimum scroll is at least +/- 1.
        GLFW.wheelPos += delta;
  
        if (!GLFW.active || !GLFW.active.scrollFunc || event.target != Browser.getCanvas()) return;
        var sx = 0;
        var sy = delta;
        if (event.type == 'mousewheel') {
          sx = event.wheelDeltaX;
        } else {
          sx = event.deltaX;
        }
  
        getWasmTableEntry(GLFW.active.scrollFunc)(GLFW.active.id, sx, sy);
  
        event.preventDefault();
      },
  onCanvasResize:(width, height, framebufferWidth, framebufferHeight) => {
        if (!GLFW.active) return;
  
        var resizeNeeded = false;
  
        // If the client is requesting fullscreen mode
        if (getFullscreenElement()) {
          if (!GLFW.active.fullscreen) {
            resizeNeeded = width != screen.width || height != screen.height;
            GLFW.active.storedX = GLFW.active.x;
            GLFW.active.storedY = GLFW.active.y;
            GLFW.active.storedWidth = GLFW.active.width;
            GLFW.active.storedHeight = GLFW.active.height;
            GLFW.active.x = GLFW.active.y = 0;
            GLFW.active.width = screen.width;
            GLFW.active.height = screen.height;
            GLFW.active.fullscreen = true;
          }
        // If the client is reverting from fullscreen mode
        } else if (GLFW.active.fullscreen == true) {
          resizeNeeded = width != GLFW.active.storedWidth || height != GLFW.active.storedHeight;
          GLFW.active.x = GLFW.active.storedX;
          GLFW.active.y = GLFW.active.storedY;
          GLFW.active.width = GLFW.active.storedWidth;
          GLFW.active.height = GLFW.active.storedHeight;
          GLFW.active.fullscreen = false;
        }
  
        if (resizeNeeded) {
          // width or height is changed (fullscreen / exit fullscreen) which will call this listener back
          // with proper framebufferWidth/framebufferHeight
          Browser.setCanvasSize(GLFW.active.width, GLFW.active.height);
        } else if (GLFW.active.width != width ||
                   GLFW.active.height != height ||
                   GLFW.active.framebufferWidth != framebufferWidth ||
                   GLFW.active.framebufferHeight != framebufferHeight) {
          GLFW.active.width = width;
          GLFW.active.height = height;
          GLFW.active.framebufferWidth = framebufferWidth;
          GLFW.active.framebufferHeight = framebufferHeight;
          GLFW.onWindowSizeChanged();
          GLFW.onFramebufferSizeChanged();
        }
      },
  onWindowSizeChanged:() => {
        if (!GLFW.active) return;
  
        if (GLFW.active.windowSizeFunc) {
          getWasmTableEntry(GLFW.active.windowSizeFunc)(GLFW.active.id, GLFW.active.width, GLFW.active.height);
        }
      },
  onFramebufferSizeChanged:() => {
        if (!GLFW.active) return;
  
        if (GLFW.active.framebufferSizeFunc) {
          getWasmTableEntry(GLFW.active.framebufferSizeFunc)(GLFW.active.id, GLFW.active.framebufferWidth, GLFW.active.framebufferHeight);
        }
      },
  onWindowContentScaleChanged:(scale) => {
        GLFW.scale = scale;
        if (!GLFW.active) return;
  
        if (GLFW.active.windowContentScaleFunc) {
          getWasmTableEntry(GLFW.active.windowContentScaleFunc)(GLFW.active.id, GLFW.scale, GLFW.scale);
        }
      },
  getTime:() => _emscripten_get_now() / 1000,
  setWindowTitle:(winid, title) => {
        var win = GLFW.WindowFromId(winid);
        if (!win) return;
  
        win.title = title;
        if (GLFW.active.id == win.id) {
          _emscripten_set_window_title(title);
        }
      },
  setJoystickCallback:(cbfun) => {
        var prevcbfun = GLFW.joystickFunc;
        GLFW.joystickFunc = cbfun;
        GLFW.refreshJoysticks();
        return prevcbfun;
      },
  joys:{
  },
  lastGamepadState:[],
  lastGamepadStateFrame:null,
  refreshJoysticks:() => {
        // Produce a new Gamepad API sample if we are ticking a new game frame, or if not using emscripten_set_main_loop() at all to drive animation.
        if (MainLoop.currentFrameNumber !== GLFW.lastGamepadStateFrame || !MainLoop.currentFrameNumber) {
          GLFW.lastGamepadState = navigator.getGamepads?.() ?? [];
          GLFW.lastGamepadStateFrame = MainLoop.currentFrameNumber;
  
          for (var joy = 0; joy < GLFW.lastGamepadState.length; ++joy) {
            var gamepad = GLFW.lastGamepadState[joy];
  
            if (gamepad) {
              if (!GLFW.joys[joy]) {
                out('glfw joystick connected:',joy);
                GLFW.joys[joy] = {
                  id: stringToNewUTF8(gamepad.id),
                  buttonsCount: gamepad.buttons.length,
                  axesCount: gamepad.axes.length,
                  buttons: _malloc(gamepad.buttons.length),
                  axes: _malloc(gamepad.axes.length*4),
                };
  
                if (GLFW.joystickFunc) {
                  getWasmTableEntry(GLFW.joystickFunc)(joy, 0x00040001); // GLFW_CONNECTED
                }
              }
  
              var data = GLFW.joys[joy];
  
              for (var i = 0; i < gamepad.buttons.length;  ++i) {
                HEAP8[data.buttons + i] = gamepad.buttons[i].pressed;
              }
  
              for (var i = 0; i < gamepad.axes.length; ++i) {
                HEAPF32[((data.axes + i*4)>>2)] = gamepad.axes[i];
              }
            } else {
              if (GLFW.joys[joy]) {
                out('glfw joystick disconnected',joy);
  
                if (GLFW.joystickFunc) {
                  getWasmTableEntry(GLFW.joystickFunc)(joy, 0x00040002); // GLFW_DISCONNECTED
                }
  
                _free(GLFW.joys[joy].id);
                _free(GLFW.joys[joy].buttons);
                _free(GLFW.joys[joy].axes);
  
                delete GLFW.joys[joy];
              }
            }
          }
        }
      },
  setKeyCallback:(winid, cbfun) => {
        var win = GLFW.WindowFromId(winid);
        if (!win) return null;
        var prevcbfun = win.keyFunc;
        win.keyFunc = cbfun;
        return prevcbfun;
      },
  setCharCallback:(winid, cbfun) => {
        var win = GLFW.WindowFromId(winid);
        if (!win) return null;
        var prevcbfun = win.charFunc;
        win.charFunc = cbfun;
        return prevcbfun;
      },
  setMouseButtonCallback:(winid, cbfun) => {
        var win = GLFW.WindowFromId(winid);
        if (!win) return null;
        var prevcbfun = win.mouseButtonFunc;
        win.mouseButtonFunc = cbfun;
        return prevcbfun;
      },
  setCursorPosCallback:(winid, cbfun) => {
        var win = GLFW.WindowFromId(winid);
        if (!win) return null;
        var prevcbfun = win.cursorPosFunc;
        win.cursorPosFunc = cbfun;
        return prevcbfun;
      },
  setScrollCallback:(winid, cbfun) => {
        var win = GLFW.WindowFromId(winid);
        if (!win) return null;
        var prevcbfun = win.scrollFunc;
        win.scrollFunc = cbfun;
        return prevcbfun;
      },
  setDropCallback:(winid, cbfun) => {
        var win = GLFW.WindowFromId(winid);
        if (!win) return null;
        var prevcbfun = win.dropFunc;
        win.dropFunc = cbfun;
        return prevcbfun;
      },
  onDrop:(event) => {
        if (!GLFW.active || !GLFW.active.dropFunc) return;
        if (!event.dataTransfer || !event.dataTransfer.files || event.dataTransfer.files.length == 0) return;
  
        event.preventDefault();
  
        var drop_dir = '.glfw_dropped_files';
        var filenames = _malloc(event.dataTransfer.files.length * 4);
        var filenamesArray = [];
        for (var i = 0; i < event.dataTransfer.files.length; ++i) {
          var path = `/${drop_dir}/${event.dataTransfer.files[i].name.replace(/\//g, '_')}`;
          var filename = stringToNewUTF8(path);
          filenamesArray.push(filename);
          HEAPU32[(((filenames)+(i*4))>>2)] = filename;
        }
  
        // Read and save the files to emscripten's FS
        var written = 0;
        FS.createPath('/', drop_dir);
  
        function save(file, in_path, numfiles) {
          var path = '/' + drop_dir + in_path + '/' + file.name.replace(/\//g, '_');
          var reader = new FileReader();
          reader.onloadend = (e) => {
            if (reader.readyState != 2) { // not DONE
              ++written;
              err(`failed to read dropped file: ${in_path}/${file.name}: ${reader.error}`);
              return;
            }
  
            var data = e.target.result;
            FS.writeFile(path, new Uint8Array(data));
            if (++written === numfiles) {
              getWasmTableEntry(GLFW.active.dropFunc)(GLFW.active.id, filenamesArray.length, filenames);
  
              for (var i = 0; i < filenamesArray.length; ++i) {
                _free(filenamesArray[i]);
              }
              _free(filenames);
            }
          };
          reader.readAsArrayBuffer(file);
        }
  
        let filesQ = [];
        function finalize() {
          var count = filesQ.length;
          for (var i = 0; i < count; ++i) {
            save(filesQ[i].file, filesQ[i].path, count);
          }
        } 
  
        if (DataTransferItem.prototype.webkitGetAsEntry) {
          let entriesTree = {};
          function markDone(fullpath, recursive) {
            if (entriesTree[fullpath].subpaths.length != 0) return;
            delete entriesTree[fullpath];
            let parentpath = fullpath.substring(0, fullpath.lastIndexOf('/'));
            if (!entriesTree.hasOwnProperty(parentpath)) {
              if (Object.keys(entriesTree).length == 0) finalize();
              return;
            }
            const fpIndex = entriesTree[parentpath].subpaths.indexOf(fullpath);
            if (fpIndex > -1) entriesTree[parentpath].subpaths.splice(fpIndex, 1);
            if (recursive) markDone(parentpath, true);
            if (Object.keys(entriesTree).length == 0) finalize();
          }
          function processEntry(entry) {
            let fp = entry.fullPath;
            let pp = fp.substring(0, fp.lastIndexOf('/'));
            entriesTree[fp] = { subpaths: [] };
            if (entry.isFile) {
              entry.file((f) => { filesQ.push({ file: f, path: pp }); markDone(fp, false); })
            } else if (entry.isDirectory) {
              if (entriesTree.hasOwnProperty(pp)) entriesTree[pp].subpaths.push(fp);
              FS.createPath('/' + drop_dir + pp, entry.name);
              var reader = entry.createReader();
              var rRead = function (dirEntries) {
                if (dirEntries.length == 0) {
                  markDone(fp, true);
                  return;
                }
                for (const ent of dirEntries) processEntry(ent);
                reader.readEntries(rRead);
              };
              reader.readEntries(rRead);
            }
          }
          for (const item of event.dataTransfer.items) {
            processEntry(item.webkitGetAsEntry());
          }
        } else {
          // fallback for browsers that does not support webkitGetAsEntry
          for (const file of event.dataTransfer.files) {
            filesQ.push({ file: file, path: '' });
          }
          finalize();
        }
  
        return false;
      },
  onDragover:(event) => {
        if (!GLFW.active || !GLFW.active.dropFunc) return;
  
        event.preventDefault();
        return false;
      },
  setWindowSizeCallback:(winid, cbfun) => {
        var win = GLFW.WindowFromId(winid);
        if (!win) return null;
        var prevcbfun = win.windowSizeFunc;
        win.windowSizeFunc = cbfun;
  
        return prevcbfun;
      },
  setWindowCloseCallback:(winid, cbfun) => {
        var win = GLFW.WindowFromId(winid);
        if (!win) return null;
        var prevcbfun = win.windowCloseFunc;
        win.windowCloseFunc = cbfun;
        return prevcbfun;
      },
  setWindowRefreshCallback:(winid, cbfun) => {
        var win = GLFW.WindowFromId(winid);
        if (!win) return null;
        var prevcbfun = win.windowRefreshFunc;
        win.windowRefreshFunc = cbfun;
        return prevcbfun;
      },
  onClickRequestPointerLock:(e) => {
        var canvas = Browser.getCanvas();
        if (!Browser.pointerLock && canvas.requestPointerLock) {
          canvas.requestPointerLock();
          e.preventDefault();
        }
      },
  setInputMode:(winid, mode, value) => {
        var win = GLFW.WindowFromId(winid);
        if (!win) return;
  
        switch (mode) {
          case 0x00033001: { // GLFW_CURSOR
            var canvas = Browser.getCanvas();
            switch (value) {
              case 0x00034001: { // GLFW_CURSOR_NORMAL
                win.inputModes[mode] = value;
                canvas.removeEventListener('click', GLFW.onClickRequestPointerLock, true);
                document.exitPointerLock();
                break;
              }
              case 0x00034002: { // GLFW_CURSOR_HIDDEN
                err('glfwSetInputMode called with GLFW_CURSOR_HIDDEN value not implemented');
                break;
              }
              case 0x00034003: { // GLFW_CURSOR_DISABLED
                win.inputModes[mode] = value;
                canvas.addEventListener('click', GLFW.onClickRequestPointerLock, true);
                canvas.requestPointerLock();
                break;
              }
              default: {
                err(`glfwSetInputMode called with unknown value parameter value: ${value}`);
                break;
              }
            }
            break;
          }
          case 0x00033002: { // GLFW_STICKY_KEYS
            err('glfwSetInputMode called with GLFW_STICKY_KEYS mode not implemented');
            break;
          }
          case 0x00033003: { // GLFW_STICKY_MOUSE_BUTTONS
            err('glfwSetInputMode called with GLFW_STICKY_MOUSE_BUTTONS mode not implemented');
            break;
          }
          case 0x00033004: { // GLFW_LOCK_KEY_MODS
            err('glfwSetInputMode called with GLFW_LOCK_KEY_MODS mode not implemented');
            break;
          }
          case 0x00033005: { // GLFW_RAW_MOUSE_MOTION
            err('glfwSetInputMode called with GLFW_RAW_MOUSE_MOTION mode not implemented');
            break;
          }
          default: {
            err(`glfwSetInputMode called with unknown mode parameter value: ${mode}`);
            break;
          }
        }
      },
  getKey:(winid, key) => {
        var win = GLFW.WindowFromId(winid);
        if (!win) return 0;
        return win.keys[key];
      },
  getMouseButton:(winid, button) => {
        var win = GLFW.WindowFromId(winid);
        if (!win) return 0;
        return (win.buttons & (1 << button)) > 0;
      },
  getCursorPos:(winid, x, y) => {
        HEAPF64[((x)>>3)] = Browser.mouseX;
        HEAPF64[((y)>>3)] = Browser.mouseY;
      },
  getMousePos:(winid, x, y) => {
        HEAP32[((x)>>2)] = Browser.mouseX;
        HEAP32[((y)>>2)] = Browser.mouseY;
      },
  setCursorPos:(winid, x, y) => {
      },
  getWindowPos:(winid, x, y) => {
        var wx = 0;
        var wy = 0;
  
        var win = GLFW.WindowFromId(winid);
        if (win) {
          wx = win.x;
          wy = win.y;
        }
  
        if (x) {
          HEAP32[((x)>>2)] = wx;
        }
  
        if (y) {
          HEAP32[((y)>>2)] = wy;
        }
      },
  setWindowPos:(winid, x, y) => {
        var win = GLFW.WindowFromId(winid);
        if (!win) return;
        win.x = x;
        win.y = y;
      },
  getWindowSize:(winid, width, height) => {
        var ww = 0;
        var wh = 0;
  
        var win = GLFW.WindowFromId(winid);
        if (win) {
          ww = win.width;
          wh = win.height;
        }
  
        if (width) {
          HEAP32[((width)>>2)] = ww;
        }
  
        if (height) {
          HEAP32[((height)>>2)] = wh;
        }
      },
  setWindowSize:(winid, width, height) => {
        var win = GLFW.WindowFromId(winid);
        if (!win) return;
  
        if (GLFW.active.id == win.id) {
          Browser.setCanvasSize(width, height); // triggers the listener (onCanvasResize) + windowSizeFunc
        }
      },
  defaultWindowHints:() => {
        GLFW.hints = {...GLFW.defaultHints};
      },
  createWindow:(width, height, title, monitor, share) => {
        var i, id;
        for (i = 0; i < GLFW.windows.length && GLFW.windows[i] !== null; i++) {
          // no-op
        }
        if (i > 0) abort('glfwCreateWindow only supports one window at time currently');
  
        // id for window
        id = i + 1;
  
        // not valid
        if (width <= 0 || height <= 0) return 0;
  
        if (monitor) {
          Browser.requestFullscreen();
        } else {
          Browser.setCanvasSize(width, height);
        }
  
        // Create context when there are no existing alive windows
        for (i = 0; i < GLFW.windows.length && GLFW.windows[i] == null; i++) {
          // no-op
        }
  
        const canvas = Browser.getCanvas();
  
        var useWebGL = GLFW.hints[0x00022001] > 0; // Use WebGL when we are told to based on GLFW_CLIENT_API
        if (i == GLFW.windows.length) {
          if (useWebGL) {
            var contextAttributes = {
              antialias: (GLFW.hints[0x0002100D] > 1), // GLFW_SAMPLES
              depth: (GLFW.hints[0x00021005] > 0),     // GLFW_DEPTH_BITS
              stencil: (GLFW.hints[0x00021006] > 0),   // GLFW_STENCIL_BITS
              alpha: (GLFW.hints[0x00021004] > 0)      // GLFW_ALPHA_BITS
            }
            Browser.createContext(canvas, /*useWebGL=*/true, /*setInModule=*/true, contextAttributes);
          } else {
            Browser.init();
          }
        }
  
        // If context creation failed, do not return a valid window
        if (!Module['ctx'] && useWebGL) return 0;
  
        // Initializes the framebuffer size from the canvas
        var win = new GLFW_Window(id, width, height, canvas.width, canvas.height, title, monitor, share);
  
        // Set window to array
        if (id - 1 == GLFW.windows.length) {
          GLFW.windows.push(win);
        } else {
          GLFW.windows[id - 1] = win;
        }
  
        GLFW.active = win;
        GLFW.adjustCanvasDimensions();
        return win.id;
      },
  destroyWindow:(winid) => {
        var win = GLFW.WindowFromId(winid);
        if (!win) return;
  
        if (win.windowCloseFunc) {
          getWasmTableEntry(win.windowCloseFunc)(win.id);
        }
  
        GLFW.windows[win.id - 1] = null;
        if (GLFW.active.id == win.id) {
          GLFW.active = null;
        }
  
        // Destroy context when no alive windows
        for (win of GLFW.windows) {
          if (win !== null) return;
        }
  
        delete Module['ctx'];
      },
  swapBuffers:(winid) => {
      },
  requestFullscreen(lockPointer, resizeCanvas) {
        Browser.lockPointer = lockPointer;
        Browser.resizeCanvas = resizeCanvas;
        if (typeof Browser.lockPointer == 'undefined') Browser.lockPointer = true;
        if (typeof Browser.resizeCanvas == 'undefined') Browser.resizeCanvas = false;
  
        var canvas = Browser.getCanvas();
        function fullscreenChange() {
          Browser.isFullscreen = false;
          var canvasContainer = canvas.parentNode;
          if (getFullscreenElement() === canvasContainer) {
            canvas.exitFullscreen = Browser.exitFullscreen;
            if (Browser.lockPointer) canvas.requestPointerLock();
            Browser.isFullscreen = true;
            if (Browser.resizeCanvas) {
              Browser.setFullscreenCanvasSize();
            } else {
              Browser.updateCanvasDimensions(canvas);
              Browser.updateResizeListeners();
            }
          } else {
            // remove the full screen specific parent of the canvas again to restore the HTML structure from before going full screen
            canvasContainer.parentNode.insertBefore(canvas, canvasContainer);
            canvasContainer.parentNode.removeChild(canvasContainer);
  
            if (Browser.resizeCanvas) {
              Browser.setWindowedCanvasSize();
            } else {
              Browser.updateCanvasDimensions(canvas);
              Browser.updateResizeListeners();
            }
          }
        }
  
        if (!Browser.fullscreenHandlersInstalled) {
          Browser.fullscreenHandlersInstalled = true;
          document.addEventListener('fullscreenchange', fullscreenChange);
          document.addEventListener('webkitfullscreenchange', fullscreenChange);
        }
  
        // create a new parent to ensure the canvas has no siblings. this allows browsers to optimize full screen performance when its parent is the full screen root
        var canvasContainer = document.createElement('div');
        canvas.parentNode.insertBefore(canvasContainer, canvas);
        canvasContainer.appendChild(canvas);
  
        // use parent of canvas as full screen root to allow aspect ratio correction (Firefox stretches the root to screen size)
        // Safari didn't Element.requestFullscreen support until 16.4
        // See: https://developer.mozilla.org/en-US/docs/Web/API/Element/requestFullscreen
        /** @suppress {checkTypes} */
        canvasContainer.requestFullscreen ??= (canvasContainer.webkitRequestFullscreen ? () => canvasContainer.webkitRequestFullscreen(Element.ALLOW_KEYBOARD_INPUT) : null) ??
                                              (canvasContainer.webkitRequestFullScreen ? () => canvasContainer.webkitRequestFullScreen(Element.ALLOW_KEYBOARD_INPUT) : null);
  
        canvasContainer.requestFullscreen();
      },
  updateCanvasDimensions(canvas, wNative, hNative) {
        const scale = GLFW.getHiDPIScale();
  
        if (wNative && hNative) {
          canvas.widthNative = wNative;
          canvas.heightNative = hNative;
        } else {
          wNative = canvas.widthNative;
          hNative = canvas.heightNative;
        }
        var w = wNative;
        var h = hNative;
        if ((getFullscreenElement() === canvas.parentNode) && (typeof screen != 'undefined')) {
          var factor = Math.min(screen.width / w, screen.height / h);
          w = Math.round(w * factor);
          h = Math.round(h * factor);
        }
        if (Browser.resizeCanvas) {
          wNative = w;
          hNative = h;
        }
        const wNativeScaled = Math.floor(wNative * scale);
        const hNativeScaled = Math.floor(hNative * scale);
        if (canvas.width  != wNativeScaled) canvas.width  = wNativeScaled;
        if (canvas.height != hNativeScaled) canvas.height = hNativeScaled;
        if (typeof canvas.style != 'undefined') {
          if (!GLFW.isCSSScalingEnabled()) {
            canvas.style.setProperty( 'width', wNative + 'px', 'important');
            canvas.style.setProperty('height', hNative + 'px', 'important');
          } else {
            canvas.style.removeProperty( 'width');
            canvas.style.removeProperty('height');
          }
        }
      },
  calculateMouseCoords(pageX, pageY) {
        // Calculate the movement based on the changes
        // in the coordinates.
        const rect = Browser.getCanvas().getBoundingClientRect();
  
        var adjustedX = pageX - (window.scrollX + rect.left);
        var adjustedY = pageY - (window.scrollY + rect.top);
  
        // getBoundingClientRect() returns dimension affected by CSS, so as a result:
        // - when CSS scaling is enabled, this will fix the mouse coordinates to match the width/height of the window
        // - otherwise the CSS width/height are forced to the width/height of the GLFW window (see updateCanvasDimensions),
        //   so there is no need to adjust the position
        if (GLFW.isCSSScalingEnabled() && GLFW.active) {
          adjustedX = adjustedX * (GLFW.active.width / rect.width);
          adjustedY = adjustedY * (GLFW.active.height / rect.height);
        }
  
        return { x: adjustedX, y: adjustedY };
      },
  setWindowAttrib:(winid, attrib, value) => {
        var win = GLFW.WindowFromId(winid);
        if (!win) return;
        const isHiDPIAware = GLFW.isHiDPIAware();
        win.attributes[attrib] = value;
        if (isHiDPIAware !== GLFW.isHiDPIAware())
          GLFW.adjustCanvasDimensions();
      },
  getDevicePixelRatio() {
        return (typeof devicePixelRatio == 'number' && devicePixelRatio) || 1.0;
      },
  isHiDPIAware() {
        if (GLFW.active)
          return GLFW.active.attributes[0x0002200C] > 0; // GLFW_SCALE_TO_MONITOR
        else
          return false;
      },
  isCSSScalingEnabled() {
        return !GLFW.isHiDPIAware();
      },
  adjustCanvasDimensions() {
        if (GLFW.active) {
          Browser.updateCanvasDimensions(Browser.getCanvas(), GLFW.active.width, GLFW.active.height);
          Browser.updateResizeListeners();
        }
      },
  getHiDPIScale() {
        return GLFW.isHiDPIAware() ? GLFW.scale : 1.0;
      },
  onDevicePixelRatioChange() {
        GLFW.onWindowContentScaleChanged(GLFW.getDevicePixelRatio());
        GLFW.adjustCanvasDimensions();
      },
  GLFW2ParamToGLFW3Param:(param) => {
        var table = {
          0x00030001:0, // GLFW_MOUSE_CURSOR
          0x00030002:0, // GLFW_STICKY_KEYS
          0x00030003:0, // GLFW_STICKY_MOUSE_BUTTONS
          0x00030004:0, // GLFW_SYSTEM_KEYS
          0x00030005:0, // GLFW_KEY_REPEAT
          0x00030006:0, // GLFW_AUTO_POLL_EVENTS
          0x00020001:0, // GLFW_OPENED
          0x00020002:0, // GLFW_ACTIVE
          0x00020003:0, // GLFW_ICONIFIED
          0x00020004:0, // GLFW_ACCELERATED
          0x00020005:0x00021001, // GLFW_RED_BITS
          0x00020006:0x00021002, // GLFW_GREEN_BITS
          0x00020007:0x00021003, // GLFW_BLUE_BITS
          0x00020008:0x00021004, // GLFW_ALPHA_BITS
          0x00020009:0x00021005, // GLFW_DEPTH_BITS
          0x0002000A:0x00021006, // GLFW_STENCIL_BITS
          0x0002000B:0x0002100F, // GLFW_REFRESH_RATE
          0x0002000C:0x00021007, // GLFW_ACCUM_RED_BITS
          0x0002000D:0x00021008, // GLFW_ACCUM_GREEN_BITS
          0x0002000E:0x00021009, // GLFW_ACCUM_BLUE_BITS
          0x0002000F:0x0002100A, // GLFW_ACCUM_ALPHA_BITS
          0x00020010:0x0002100B, // GLFW_AUX_BUFFERS
          0x00020011:0x0002100C, // GLFW_STEREO
          0x00020012:0, // GLFW_WINDOW_NO_RESIZE
          0x00020013:0x0002100D, // GLFW_FSAA_SAMPLES
          0x00020014:0x00022002, // GLFW_OPENGL_VERSION_MAJOR
          0x00020015:0x00022003, // GLFW_OPENGL_VERSION_MINOR
          0x00020016:0x00022006, // GLFW_OPENGL_FORWARD_COMPAT
          0x00020017:0x00022007, // GLFW_OPENGL_DEBUG_CONTEXT
          0x00020018:0x00022008, // GLFW_OPENGL_PROFILE
        };
        return table[param];
      },
  };
  var _glfwCreateWindow = (width, height, title, monitor, share) => GLFW.createWindow(width, height, title, monitor, share);

  var _glfwDestroyWindow = (winid) => GLFW.destroyWindow(winid);

  var _glfwGetCursorPos = (winid, x, y) => GLFW.getCursorPos(winid, x, y);

  
  var _glfwGetFramebufferSize = (winid, width, height) => {
      var ww = 0;
      var wh = 0;
  
      var win = GLFW.WindowFromId(winid);
      if (win) {
        ww = win.framebufferWidth;
        wh = win.framebufferHeight;
      }
  
      if (width) {
        HEAP32[((width)>>2)] = ww;
      }
  
      if (height) {
        HEAP32[((height)>>2)] = wh;
      }
    };

  var _glfwGetInputMode = (winid, mode) => {
      var win = GLFW.WindowFromId(winid);
      if (!win) return;
  
      switch (mode) {
        case 0x00033001: { // GLFW_CURSOR
          if (Browser.pointerLock) {
            win.inputModes[mode] = 0x00034003; // GLFW_CURSOR_DISABLED
          } else {
            win.inputModes[mode] = 0x00034001; // GLFW_CURSOR_NORMAL
          }
        }
      }
  
      return win.inputModes[mode];
    };

  
  var _glfwGetJoystickButtons = (joy, count) => {
      GLFW.refreshJoysticks();
  
      var state = GLFW.joys[joy];
      if (!state || !state.buttons) {
        HEAP32[((count)>>2)] = 0;
        return;
      }
  
      HEAP32[((count)>>2)] = state.buttonsCount;
      return state.buttons;
    };

  var _glfwGetJoystickName = (joy) => {
      if (GLFW.joys[joy]) {
        return GLFW.joys[joy].id;
      }
      return 0;
    };

  var _glfwGetMonitorName = (mon) => {
      GLFW.monitorString ||= stringToNewUTF8('HTML5 WebGL Canvas');
      return GLFW.monitorString;
    };

  
  
  var _glfwGetMonitors = (count) => {
      HEAP32[((count)>>2)] = 1;
      if (!GLFW.monitors) {
        GLFW.monitors = _malloc(4);
        HEAP32[((GLFW.monitors)>>2)] = 1;
      }
      return GLFW.monitors;
    };

  var _glfwGetMouseButton = (winid, button) => GLFW.getMouseButton(winid, button);

  
  var _glfwGetVideoModes = (monitor, count) => {
      HEAP32[((count)>>2)] = 0;
      return 0;
    };

  var _glfwGetWindowSize = (winid, width, height) => GLFW.getWindowSize(winid, width, height);

  var _glfwInit = () => {
      if (GLFW.windows) return 1; // GL_TRUE
  
      GLFW.initialTime = GLFW.getTime();
      GLFW.defaultWindowHints();
      GLFW.windows = new Array()
      GLFW.active = null;
      GLFW.scale  = GLFW.getDevicePixelRatio();
  
      window.addEventListener('gamepadconnected', GLFW.onGamepadConnected, true);
      window.addEventListener('gamepaddisconnected', GLFW.onGamepadDisconnected, true);
      window.addEventListener('keydown', GLFW.onKeydown, true);
      window.addEventListener('keypress', GLFW.onKeyPress, true);
      window.addEventListener('keyup', GLFW.onKeyup, true);
      window.addEventListener('blur', GLFW.onBlur, true);
  
      // watch for devicePixelRatio changes
      GLFW.devicePixelRatioMQL = window.matchMedia('(resolution: ' + GLFW.getDevicePixelRatio() + 'dppx)');
      GLFW.devicePixelRatioMQL.addEventListener('change', GLFW.onDevicePixelRatioChange);
  
      var canvas = Browser.getCanvas();
      canvas.addEventListener('touchmove', GLFW.onMousemove, true);
      canvas.addEventListener('touchstart', GLFW.onMouseButtonDown, true);
      canvas.addEventListener('touchcancel', GLFW.onMouseButtonUp, true);
      canvas.addEventListener('touchend', GLFW.onMouseButtonUp, true);
      canvas.addEventListener('mousemove', GLFW.onMousemove, true);
      canvas.addEventListener('mousedown', GLFW.onMouseButtonDown, true);
      canvas.addEventListener('mouseup', GLFW.onMouseButtonUp, true);
      canvas.addEventListener('wheel', GLFW.onMouseWheel, true);
      canvas.addEventListener('mousewheel', GLFW.onMouseWheel, true);
      canvas.addEventListener('mouseenter', GLFW.onMouseenter, true);
      canvas.addEventListener('mouseleave', GLFW.onMouseleave, true);
      canvas.addEventListener('drop', GLFW.onDrop, true);
      canvas.addEventListener('dragover', GLFW.onDragover, true);
  
      // Overriding implementation to account for HiDPI
      Browser.requestFullscreen = GLFW.requestFullscreen;
      Browser.calculateMouseCoords = GLFW.calculateMouseCoords;
      Browser.updateCanvasDimensions = GLFW.updateCanvasDimensions;
  
      Browser.resizeListeners.push((width, height) => {
        if (GLFW.isHiDPIAware()) {
          var canvas = Browser.getCanvas();
          GLFW.onCanvasResize(canvas.clientWidth, canvas.clientHeight, width, height);
        } else {
          GLFW.onCanvasResize(width, height, width, height);
        }
      });
  
      return 1; // GL_TRUE
    };

  var _glfwJoystickPresent = (joy) => {
      GLFW.refreshJoysticks();
  
      return GLFW.joys[joy] !== undefined;
    };

  var _glfwMakeContextCurrent = (winid) => 0;

  var _glfwPollEvents = () => 0;

  var _glfwSetCursorEnterCallback = (winid, cbfun) => {
      var win = GLFW.WindowFromId(winid);
      if (!win) return null;
      var prevcbfun = win.cursorEnterFunc;
      win.cursorEnterFunc = cbfun;
      return prevcbfun;
    };

  var _glfwSetCursorPos = (winid, x, y) => GLFW.setCursorPos(winid, x, y);

  var _glfwSetCursorPosCallback = (winid, cbfun) => GLFW.setCursorPosCallback(winid, cbfun);

  var _glfwSetErrorCallback = (cbfun) => {
      var prevcbfun = GLFW.errorFunc;
      GLFW.errorFunc = cbfun;
      return prevcbfun;
    };

  var _glfwSetFramebufferSizeCallback = (winid, cbfun) => {
      var win = GLFW.WindowFromId(winid);
      if (!win) return null;
      var prevcbfun = win.framebufferSizeFunc;
      win.framebufferSizeFunc = cbfun;
      return prevcbfun;
    };

  var _glfwSetInputMode = (winid, mode, value) => {
      GLFW.setInputMode(winid, mode, value);
    };

  var _glfwSetJoystickCallback = (cbfun) => GLFW.setJoystickCallback(cbfun);

  var _glfwSetKeyCallback = (winid, cbfun) => GLFW.setKeyCallback(winid, cbfun);

  var _glfwSetScrollCallback = (winid, cbfun) => GLFW.setScrollCallback(winid, cbfun);

  var _glfwSetWindowFocusCallback = (winid, cbfun) => {
      var win = GLFW.WindowFromId(winid);
      if (!win) return null;
      var prevcbfun = win.windowFocusFunc;
      win.windowFocusFunc = cbfun;
      return prevcbfun;
    };

  var _glfwSetWindowIconifyCallback = (winid, cbfun) => {
      var win = GLFW.WindowFromId(winid);
      if (!win) return null;
      var prevcbfun = win.windowIconifyFunc;
      win.windowIconifyFunc = cbfun;
      return prevcbfun;
    };

  var _glfwSetWindowSize = (winid, width, height) => GLFW.setWindowSize(winid, width, height);

  var _glfwSwapBuffers = (winid) => GLFW.swapBuffers(winid);

  
  var _glfwSwapInterval = (interval) => {
      interval = Math.abs(interval); // GLFW uses negative values to enable GLX_EXT_swap_control_tear, which we don't have, so just treat negative and positive the same.
      if (interval == 0) _emscripten_set_main_loop_timing(0, 0);
      else _emscripten_set_main_loop_timing(1, interval);
    };

  var _glfwTerminate = () => {
      window.removeEventListener('gamepadconnected', GLFW.onGamepadConnected, true);
      window.removeEventListener('gamepaddisconnected', GLFW.onGamepadDisconnected, true);
      window.removeEventListener('keydown', GLFW.onKeydown, true);
      window.removeEventListener('keypress', GLFW.onKeyPress, true);
      window.removeEventListener('keyup', GLFW.onKeyup, true);
      window.removeEventListener('blur', GLFW.onBlur, true);
      var canvas = Browser.getCanvas();
      canvas.removeEventListener('touchmove', GLFW.onMousemove, true);
      canvas.removeEventListener('touchstart', GLFW.onMouseButtonDown, true);
      canvas.removeEventListener('touchcancel', GLFW.onMouseButtonUp, true);
      canvas.removeEventListener('touchend', GLFW.onMouseButtonUp, true);
      canvas.removeEventListener('mousemove', GLFW.onMousemove, true);
      canvas.removeEventListener('mousedown', GLFW.onMouseButtonDown, true);
      canvas.removeEventListener('mouseup', GLFW.onMouseButtonUp, true);
      canvas.removeEventListener('wheel', GLFW.onMouseWheel, true);
      canvas.removeEventListener('mousewheel', GLFW.onMouseWheel, true);
      canvas.removeEventListener('mouseenter', GLFW.onMouseenter, true);
      canvas.removeEventListener('mouseleave', GLFW.onMouseleave, true);
      canvas.removeEventListener('drop', GLFW.onDrop, true);
      canvas.removeEventListener('dragover', GLFW.onDragover, true);
  
      if (GLFW.devicePixelRatioMQL)
        GLFW.devicePixelRatioMQL.removeEventListener('change', GLFW.onDevicePixelRatioChange);
  
      canvas.width = canvas.height = 1;
      GLFW.windows = null;
      GLFW.active = null;
    };

  var _glfwWindowHint = (target, hint) => {
      GLFW.hints[target] = hint;
    };

  var _glfwWindowShouldClose = (winid) => {
      var win = GLFW.WindowFromId(winid);
      if (!win) return 0;
      return win.shouldClose;
    };




  
  
  var stackAlloc = (sz) => __emscripten_stack_alloc(sz);
  var stringToUTF8OnStack = (str) => {
      var size = lengthBytesUTF8(str) + 1;
      var ret = stackAlloc(size);
      stringToUTF8(str, ret, size);
      return ret;
    };



  var runAndAbortIfError = (func) => {
      try {
        return func();
      } catch (e) {
        abort(e);
      }
    };
  
  
  var runtimeKeepalivePush = () => {
      runtimeKeepaliveCounter += 1;
    };
  
  var runtimeKeepalivePop = () => {
      runtimeKeepaliveCounter -= 1;
    };
  var Asyncify = {
  instrumentWasmImports(imports) {
        var importPattern = /^(invoke_.*|__asyncjs__.*)$/;
  
        for (let [x, original] of Object.entries(imports)) {
          if (typeof original == 'function') {
            let isAsyncifyImport = original.isAsync || importPattern.test(x);
            // Wrap async imports with a suspending WebAssembly function.
            if (isAsyncifyImport) {
              imports[x] = original = new WebAssembly.Suspending(original);
            }
          }
        }
      },
  instrumentWasmExports(exports) {
        var exportPattern = /^(main|EmscriptenTick|ViceExt.*|main|__main_argc_argv)$/;
        Asyncify.asyncExports = new Set();
        var ret = {};
        for (let [x, original] of Object.entries(exports)) {
          if (typeof original == 'function') {
            // Wrap all exports with a promising WebAssembly function.
            let isAsyncifyExport = exportPattern.test(x);
            if (isAsyncifyExport) {
              Asyncify.asyncExports.add(original);
              original = Asyncify.makeAsyncFunction(original);
            }
            ret[x] = original;
          } else {
            ret[x] = original;
          }
        }
        return ret;
      },
  asyncExports:null,
  isAsyncExport(func) {
        return Asyncify.asyncExports?.has(func);
      },
  handleAsync:async (startAsync) => {
        
        try {
          return await startAsync();
        } finally {
          
        }
      },
  handleSleep:(startAsync) => Asyncify.handleAsync(() => new Promise(startAsync)),
  makeAsyncFunction(original) {
        return WebAssembly.promising(original);
      },
  };








































































































































































































































































































































































  var FS_createPath = (...args) => FS.createPath(...args);



  var FS_unlink = (...args) => FS.unlink(...args);

  var FS_createLazyFile = (...args) => FS.createLazyFile(...args);

  var FS_createDevice = (...args) => FS.createDevice(...args);



  FS.createPreloadedFile = FS_createPreloadedFile;
  FS.preloadFile = FS_preloadFile;
  FS.staticInit();;

      Module['requestAnimationFrame'] = MainLoop.requestAnimationFrame;
      Module['pauseMainLoop'] = MainLoop.pause;
      Module['resumeMainLoop'] = MainLoop.resume;
      MainLoop.init();;

      // Signal GL rendering layer that processing of a new frame is about to
      // start. This helps it optimize VBO double-buffering and reduce GPU stalls.
      registerPreMainLoop(() => GL.newRenderingFrameStarted());
    ;
for (let i = 0; i < 32; ++i) tempFixedLengthArray.push(new Array(i));;
var miniTempWebGLFloatBuffersStorage = new Float32Array(288);
  // Create GL_POOL_TEMP_BUFFERS_SIZE+1 temporary buffers, for uploads of size 0 through GL_POOL_TEMP_BUFFERS_SIZE inclusive
  for (/**@suppress{duplicate}*/var i = 0; i <= 288; ++i) {
    miniTempWebGLFloatBuffers[i] = miniTempWebGLFloatBuffersStorage.subarray(0, i);
  };
var miniTempWebGLIntBuffersStorage = new Int32Array(288);
  // Create GL_POOL_TEMP_BUFFERS_SIZE+1 temporary buffers, for uploads of size 0 through GL_POOL_TEMP_BUFFERS_SIZE inclusive
  for (/**@suppress{duplicate}*/var i = 0; i <= 288; ++i) {
    miniTempWebGLIntBuffers[i] = miniTempWebGLIntBuffersStorage.subarray(0, i);
  };
// End JS library code

// include: postlibrary.js
// This file is included after the automatically-generated JS library code
// but before the wasm module is created.

{

  // Begin ATMODULES hooks
  if (Module['noExitRuntime']) noExitRuntime = Module['noExitRuntime'];

if (Module['print']) out = Module['print'];
if (Module['printErr']) err = Module['printErr'];
  // End ATMODULES hooks

  if (Module['arguments']) programArgs = Module['arguments'];
  if (Module['thisProgram']) thisProgram = Module['thisProgram'];

  var preInit = Module['preInit'];
  if (preInit) {
    if (typeof preInit == 'function') Module['preInit'] = preInit = [preInit];
    // Written as a loop so that preInit functions that themselves add more
    // preInit functions.  Is this actually needed?
    while (preInit.length > 0) {
      preInit.shift()();
    }
  }
}

// Begin runtime exports
  Module['addRunDependency'] = addRunDependency;
  Module['removeRunDependency'] = removeRunDependency;
  Module['FS_preloadFile'] = FS_preloadFile;
  Module['FS_unlink'] = FS_unlink;
  Module['FS_createPath'] = FS_createPath;
  Module['FS_createDevice'] = FS_createDevice;
  Module['FS_createDataFile'] = FS_createDataFile;
  Module['FS_createLazyFile'] = FS_createLazyFile;
  // End runtime exports
  // Begin JS library exports
  // End JS library exports

// end include: postlibrary.js

var ASM_CONSTS = {
  479796: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 479881: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 479966: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 480051: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 480136: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 480221: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 480306: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 480391: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 480476: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 480561: () => { try { window.__vcInGame = false; } catch (e) {} },  
 480613: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 480698: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 480783: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 480868: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 480953: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 481038: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 481123: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 481208: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 481293: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 481378: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 481463: () => { return window.__sfxDump ? 1 : 0; },  
 481500: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 481585: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 481670: () => { try { window.__sfxDump = 0; } catch (e) {} },  
 481717: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 481802: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 481887: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 481972: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 482057: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 482142: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 482227: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 482312: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 482397: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 482482: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 482567: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 482652: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 482737: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 482822: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 482907: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 482992: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 483077: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 483162: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 483247: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 483332: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 483417: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 483502: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 483587: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 483672: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 483757: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 483842: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 483927: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 484012: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 484097: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 484182: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 484267: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 484352: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 484437: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 484522: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 484607: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 484692: () => { try { window.__vcInGame = false; } catch (e) {} },  
 484744: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 484829: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 484914: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 484999: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 485084: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 485169: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 485254: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 485339: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 485424: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 485509: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 485594: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 485679: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 485764: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 485849: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 485934: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 486019: ($0, $1) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0) + $1); } catch (e) {} },  
 486109: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 486194: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 486279: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 486364: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 486449: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 486534: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 486619: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 486704: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 486789: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 486874: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 486959: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 487044: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 487129: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 487214: () => { try { OD.syncUserfiles('settings'); } catch (e) {} },  
 487269: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 487354: () => { try { if (window.__vcSkinImport) window.__vcSkinImport(); } catch (e) {} },  
 487431: ($0) => { try { if (window.__vcSkinDelete) window.__vcSkinDelete(UTF8ToString($0)); } catch (e) {} },  
 487524: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 487609: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 487694: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 487779: () => { return window.__vcWebSaveManager | 0; },  
 487821: () => { window.__vcWebSaveManager = 0; },  
 487856: () => { return window.__vcWebSaveNoSpace === 1 ? 1 : 0; },  
 487908: () => { window.__vcWebSaveNoSpace = 0; },  
 487943: () => { return window.__vcWebSaveImported | 0; },  
 487986: () => { window.__vcWebSaveImported = 0; },  
 488022: () => { return window.__vcWebMusicImported | 0; },  
 488066: () => { window.__vcWebMusicImported = 0; },  
 488103: ($0, $1) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0) + $1); } catch (e) {} },  
 488193: () => { return window.__vcWebMusicRescan | 0; },  
 488235: () => { window.__vcWebMusicRescan = 0; },  
 488270: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 488355: () => { return window.__vcWebSkinRescan | 0; },  
 488396: () => { window.__vcWebSkinRescan = 0; },  
 488430: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 488515: () => { return window.__vcWebSkinResult | 0; },  
 488556: () => { window.__vcWebSkinResult = 0; },  
 488590: ($0, $1) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0) + $1); } catch (e) {} },  
 488680: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 488765: () => { try { if (window.__vcSkinImport) window.__vcSkinImport(); } catch (e) {} },  
 488842: ($0) => { try { if (window.__vcSaveDownload) window.__vcSaveDownload($0); } catch (e) {} },  
 488925: () => { try { if (window.__vcGalleryDownload) window.__vcGalleryDownload(); } catch (e) {} },  
 489012: () => { try { if (window.__vcGalleryClear) window.__vcGalleryClear(); } catch (e) {} },  
 489093: () => { try { if (window.__vcMusicImport) window.__vcMusicImport(); } catch (e) {} },  
 489172: () => { try { if (window.__vcMusicDownloadAll) window.__vcMusicDownloadAll(); } catch (e) {} },  
 489261: () => { try { if (window.__vcMusicDeleteAll) window.__vcMusicDeleteAll(); } catch (e) {} },  
 489346: () => { try { if (window.__vcSaveDownloadAll) window.__vcSaveDownloadAll(); } catch (e) {} },  
 489433: () => { try { if (window.__vcSaveImport) window.__vcSaveImport(); } catch (e) {} },  
 489510: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 489595: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 489680: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 489765: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 489850: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 489935: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 490020: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 490105: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 490190: ($0, $1) => { if (window.__loadProgress) window.__loadProgress($0, $1); },  
 490252: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 490337: ($0) => { try { OD.ensure(UTF8ToString($0)); } catch (e) {} },  
 490391: ($0, $1) => { if (window.__loadProgress) window.__loadProgress($0, $1); },  
 490453: ($0, $1) => { if (window.__loadProgress) window.__loadProgress($0, $1); },  
 490515: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 490600: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 490685: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 490770: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 490855: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 490940: ($0, $1) => { if (window.__loadProgress) window.__loadProgress($0, $1); },  
 491002: ($0, $1) => { if (window.__loadProgress) window.__loadProgress($0, $1); },  
 491064: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 491149: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 491234: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 491319: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 491404: ($0, $1, $2, $3) => { try { var gp = navigator.getGamepads ? navigator.getGamepads()[$0] : null; if (!gp || !gp.vibrationActuator) return; var p; if ($1 == 0 && $2 == 0) p = gp.vibrationActuator.reset ? gp.vibrationActuator.reset() : null; else p = gp.vibrationActuator.playEffect('dual-rumble', { startDelay: 0, duration: $3, strongMagnitude: $1 / 255, weakMagnitude: $2 / 255 }); if (p && p.catch) p.catch(function () {}); } catch (e) {} },  
 491826: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 491911: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 491996: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 492081: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 492166: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 492251: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 492336: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 492421: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 492506: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 492591: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 492676: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 492761: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 492846: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 492931: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 493016: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 493101: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 493186: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 493271: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 493356: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 493441: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 493526: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 493611: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 493696: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 493781: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 493866: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 493951: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 494036: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 494121: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 494206: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 494291: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 494376: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 494461: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 494546: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 494631: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 494716: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 494801: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 494886: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 494971: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 495056: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 495141: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 495226: ($0) => { try { OD.movingEnts = $0; } catch (e) {} },  
 495271: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 495356: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 495441: () => { try { var FSx = (typeof FS !== 'undefined') ? FS : (typeof Module !== 'undefined' && Module.FS); if (!FSx) return 1; try { FSx.mkdir('/userfiles'); } catch (e) {} try { FSx.mkdir('/userfiles/Gallery'); } catch (e) {} var max = -1; var re = /^photo_(d+).png$/; var list = FSx.readdir('/userfiles/Gallery'); for (var i = 0; i < list.length; i++) { var m = re.exec(list[i]); if (m) max = Math.max(max, parseInt(m[1], 10) || 0); } return max + 1; } catch (e) { return 1; } },  
 495914: () => { try { if (typeof OD !== 'undefined' && OD.syncUserfiles) OD.syncUserfiles('gallery'); else if (typeof FS !== 'undefined') FS.syncfs(false, function () {}); } catch (e) {} },  
 496089: ($0) => { try { if (window.__odEnt) window.__odEnt($0); } catch (e) {} },  
 496154: ($0) => { try { if (window.__odMark) window.__odMark($0); } catch (e) {} },  
 496221: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 496306: () => { var g = (typeof globalThis !== 'undefined') ? globalThis : window; if (typeof g.__vcShowFps !== 'undefined') return g.__vcShowFps ? 1 : 0; return 0; },  
 496459: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 496544: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 496629: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 496714: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 496799: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 496884: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 496969: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 497054: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 497139: ($0) => { try { OD.ensure(UTF8ToString($0)); } catch (e) {} },  
 497193: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 497278: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 497363: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 497448: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 497533: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 497618: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 497703: ($0, $1) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0) + $1); } catch (e) {} },  
 497793: ($0, $1) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0) + $1); } catch (e) {} },  
 497883: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 497968: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 498053: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 498138: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 498223: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 498308: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 498393: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 498478: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 498563: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 498648: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 498733: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 498818: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 498903: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 498988: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 499073: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 499158: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 499243: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 499328: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 499413: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 499498: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 499583: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 499668: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 499753: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 499838: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 499923: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 500008: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 500093: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 500178: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 500263: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 500348: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 500433: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 500518: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 500603: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 500688: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 500773: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 500858: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 500943: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 501028: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 501113: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 501198: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 501283: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 501368: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 501453: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 501538: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 501623: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 501708: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 501793: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 501878: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 501963: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 502048: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 502133: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 502218: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 502303: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 502388: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 502473: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 502558: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 502643: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 502728: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 502813: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 502898: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 502983: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 503068: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 503153: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 503238: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 503323: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 503408: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 503493: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 503578: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 503663: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 503748: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 503833: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 503918: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 504003: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 504088: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 504173: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 504258: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 504343: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 504428: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 504513: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 504598: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 504683: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 504768: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 504853: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 504938: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 505023: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 505108: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 505193: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 505278: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 505363: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 505448: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 505533: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 505618: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 505703: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 505788: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 505873: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 505958: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 506043: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 506128: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 506213: ($0) => { if (window.__vcPedClumpFatal) window.__vcPedClumpFatal($0); },  
 506277: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 506362: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 506447: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 506532: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 506617: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 506702: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 506787: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 506872: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 506957: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 507042: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 507127: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 507212: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 507297: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 507382: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 507467: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 507552: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 507637: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 507722: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 507807: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 507892: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 507977: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 508062: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 508147: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 508232: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 508317: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 508402: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 508487: () => { try { OD.syncUserfiles('save'); } catch (e) { console.log('[od] flush save no disponible'); } },  
 508585: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 508670: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 508755: () => { return window.innerWidth|0; },  
 508787: () => { return window.innerHeight|0; },  
 508820: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 508905: ($0, $1) => { try { window.__vcState = $0; window.__vcInGame = ($1 === 1); } catch (e) {} },  
 508985: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 509070: () => { return OD.census().n; },  
 509096: () => { return OD.census().mb; },  
 509123: () => { return OD.census().live; },  
 509152: () => { return OD.census().ev; },  
 509179: () => { return OD.census().evmb; },  
 509208: () => { return OD.census().fmb; },  
 509236: () => { return OD.census().idb; },  
 509264: () => { return OD.census().ufs; },  
 509292: () => { return (typeof OD !== 'undefined' && OD.movingEnts) ? OD.movingEnts : -1; },  
 509370: () => { var c = document.querySelector(".vc-canvas"); return c ? c.width : -1; },  
 509445: () => { var c = document.querySelector(".vc-canvas"); return c ? c.height : -1; },  
 509521: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 509606: () => { try { if (typeof location !== 'undefined' && /oddebug/.test(location.search)) { var c = OD.census(); if (c.per) console.log('[od] perdir ' + c.per); if (c.big) console.log('[od] big ' + c.big); } } catch (e) {} },  
 509821: () => { try { if (window.__vcFrame) window.__vcFrame(); } catch (e) {} },  
 509888: () => { return (window.__vcIdbReady === undefined || window.__vcIdbReady) ? 1 : 0; },  
 509967: () => { try { (window.__odq = window.__odq || []).push('INI boot save'); } catch (e) {} },  
 510051: () => { try { (window.__odq = window.__odq || []).push('INI boot save omitido: idbfs sin poblar'); } catch (e) {} },  
 510161: () => { return /[?&]heapprobe=1/.test(location.search) ? 1 : 0; },  
 510221: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 510306: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 510391: () => { try { if ((window.__vcODBlock | 0) > 0) return -1; return window.__vcRaf ? (window.__vcRaf.n | 0) : -1; } catch (e) { return -1; } },  
 510526: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 510611: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 510696: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 510781: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 510866: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 510951: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 511036: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 511121: () => { try { window.__vcODBlock = ((window.__vcODBlock | 0) + 1); } catch (e) {} },  
 511199: () => { try { window.__vcODBlock = Math.max(0, ((window.__vcODBlock | 0) - 1)); } catch (e) {} },  
 511290: () => { try { window.__quitting = true; } catch (e) {} try { OD.syncUserfiles('quit'); setTimeout(function(){ location.reload(); }, 1200); } catch (e) { location.reload(); } },  
 511460: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 511545: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 511630: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 511715: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 511800: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 511885: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 511970: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 512055: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 512140: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 512225: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 512310: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 512395: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 512480: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 512565: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 512650: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 512735: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 512820: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 512905: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 512990: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 513075: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 513160: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 513245: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 513330: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 513415: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 513500: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 513585: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 513670: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 513755: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 513840: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 513925: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 514010: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 514095: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 514180: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 514265: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 514350: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 514435: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 514520: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 514605: ($0, $1) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0) + UTF8ToString($1)); } catch (e) {} },  
 514709: ($0) => { try { (window.__odq = window.__odq || []).push('[texconv] ' + UTF8ToString($0)); } catch (e) {} },  
 514809: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 514894: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 514979: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 515064: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} },  
 515149: ($0) => { try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} }
};
function __asyncjs__od_fetch(cpath) { return Asyncify.handleAsync(async () => { const p = UTF8ToString(cpath); try { try { window.__vcFetchNow = p; } catch (e) {} const r = await OD.ensure(p); try { window.__vcFetchNow = null; } catch (e) {} if (!r) return 0; const len = lengthBytesUTF8(r) + 1; const ptr = _malloc(len); stringToUTF8(r, ptr, len); return ptr; } catch (e) { return 0; } }); }
__asyncjs__od_fetch.sig = 'ii';
function od_present(cpath) { const p = UTF8ToString(cpath); try { const r = OD.presentSync(p); if (!r) return 0; const len = lengthBytesUTF8(r) + 1; const ptr = _malloc(len); stringToUTF8(r, ptr, len); return ptr; } catch (e) { return 0; } }
od_present.sig = 'ii';
function od_sfx_ready(cpath) { const p = UTF8ToString(cpath); try { if (OD.presentSync(p)) return 1; OD._ensure(p).catch(function(){}); return 0; } catch (e) { return 0; } }
od_sfx_ready.sig = 'ii';

// Imports from the Wasm binary.
var _free,
  _malloc,
  _ViceExtRescanUserMusic,
  _ViceExtRescanSkins,
  _ViceExtRescanSaves,
  _ViceExtTouchFrame,
  _ViceExtTouchSetEnabled,
  _ViceExtTouchSetAutoaim,
  _ViceExtTouchContext,
  _ViceExtTouchMenuActivate,
  _ViceExtTouchMenuChange,
  _ViceExtTouchMenuStep,
  _ViceExtTouchSkip,
  _ViceExtTouchUseTouchIcons,
  _ViceExtTouchMenuOption,
  _ViceExtTouchPadMode,
  _ViceExtTouchPause,
  _ViceExtTouchOpenMap,
  _ViceExtTouchWeapon,
  _ViceExtTouchRadio,
  _ViceExtTouchVehicleType,
  _ViceExtTouchNearVehicleType,
  _ViceExtTouchArmedVehicle,
  _ViceExtTouchMissionVehicle,
  _ViceExtTouchAutoAim,
  _ViceExtTextTrace,
  _EmscriptenTick,
  _main,
  __emscripten_stack_restore,
  __emscripten_stack_alloc,
  _emscripten_stack_get_current,
  memory,
  __indirect_function_table,
  wasmMemory,
  wasmTable;


function assignWasmExports(wasmExports) {
  _free = wasmExports['free'];
  _malloc = wasmExports['malloc'];
  _ViceExtRescanUserMusic = Module['_ViceExtRescanUserMusic'] = wasmExports['ViceExtRescanUserMusic'];
  _ViceExtRescanSkins = Module['_ViceExtRescanSkins'] = wasmExports['ViceExtRescanSkins'];
  _ViceExtRescanSaves = Module['_ViceExtRescanSaves'] = wasmExports['ViceExtRescanSaves'];
  _ViceExtTouchFrame = Module['_ViceExtTouchFrame'] = wasmExports['ViceExtTouchFrame'];
  _ViceExtTouchSetEnabled = Module['_ViceExtTouchSetEnabled'] = wasmExports['ViceExtTouchSetEnabled'];
  _ViceExtTouchSetAutoaim = Module['_ViceExtTouchSetAutoaim'] = wasmExports['ViceExtTouchSetAutoaim'];
  _ViceExtTouchContext = Module['_ViceExtTouchContext'] = wasmExports['ViceExtTouchContext'];
  _ViceExtTouchMenuActivate = Module['_ViceExtTouchMenuActivate'] = wasmExports['ViceExtTouchMenuActivate'];
  _ViceExtTouchMenuChange = Module['_ViceExtTouchMenuChange'] = wasmExports['ViceExtTouchMenuChange'];
  _ViceExtTouchMenuStep = Module['_ViceExtTouchMenuStep'] = wasmExports['ViceExtTouchMenuStep'];
  _ViceExtTouchSkip = Module['_ViceExtTouchSkip'] = wasmExports['ViceExtTouchSkip'];
  _ViceExtTouchUseTouchIcons = Module['_ViceExtTouchUseTouchIcons'] = wasmExports['ViceExtTouchUseTouchIcons'];
  _ViceExtTouchMenuOption = Module['_ViceExtTouchMenuOption'] = wasmExports['ViceExtTouchMenuOption'];
  _ViceExtTouchPadMode = Module['_ViceExtTouchPadMode'] = wasmExports['ViceExtTouchPadMode'];
  _ViceExtTouchPause = Module['_ViceExtTouchPause'] = wasmExports['ViceExtTouchPause'];
  _ViceExtTouchOpenMap = Module['_ViceExtTouchOpenMap'] = wasmExports['ViceExtTouchOpenMap'];
  _ViceExtTouchWeapon = Module['_ViceExtTouchWeapon'] = wasmExports['ViceExtTouchWeapon'];
  _ViceExtTouchRadio = Module['_ViceExtTouchRadio'] = wasmExports['ViceExtTouchRadio'];
  _ViceExtTouchVehicleType = Module['_ViceExtTouchVehicleType'] = wasmExports['ViceExtTouchVehicleType'];
  _ViceExtTouchNearVehicleType = Module['_ViceExtTouchNearVehicleType'] = wasmExports['ViceExtTouchNearVehicleType'];
  _ViceExtTouchArmedVehicle = Module['_ViceExtTouchArmedVehicle'] = wasmExports['ViceExtTouchArmedVehicle'];
  _ViceExtTouchMissionVehicle = Module['_ViceExtTouchMissionVehicle'] = wasmExports['ViceExtTouchMissionVehicle'];
  _ViceExtTouchAutoAim = Module['_ViceExtTouchAutoAim'] = wasmExports['ViceExtTouchAutoAim'];
  _ViceExtTextTrace = Module['_ViceExtTextTrace'] = wasmExports['ViceExtTextTrace'];
  _EmscriptenTick = Module['_EmscriptenTick'] = wasmExports['EmscriptenTick'];
  _main = Module['_main'] = wasmExports['__main_argc_argv'];
  __emscripten_stack_restore = wasmExports['_emscripten_stack_restore'];
  __emscripten_stack_alloc = wasmExports['_emscripten_stack_alloc'];
  _emscripten_stack_get_current = wasmExports['emscripten_stack_get_current'];
  memory = wasmMemory = wasmExports['memory'];
  __indirect_function_table = wasmTable = wasmExports['__indirect_function_table'];
}

var wasmImports = {
  /** @export */
  __asyncjs__od_fetch,
  /** @export */
  __call_sighandler: ___call_sighandler,
  /** @export */
  __cxa_throw: ___cxa_throw,
  /** @export */
  __syscall_chdir: ___syscall_chdir,
  /** @export */
  __syscall_faccessat: ___syscall_faccessat,
  /** @export */
  __syscall_fcntl64: ___syscall_fcntl64,
  /** @export */
  __syscall_fstat64: ___syscall_fstat64,
  /** @export */
  __syscall_getcwd: ___syscall_getcwd,
  /** @export */
  __syscall_getdents64: ___syscall_getdents64,
  /** @export */
  __syscall_ioctl: ___syscall_ioctl,
  /** @export */
  __syscall_lstat64: ___syscall_lstat64,
  /** @export */
  __syscall_mkdirat: ___syscall_mkdirat,
  /** @export */
  __syscall_newfstatat: ___syscall_newfstatat,
  /** @export */
  __syscall_openat: ___syscall_openat,
  /** @export */
  __syscall_poll: ___syscall_poll,
  /** @export */
  __syscall_readlinkat: ___syscall_readlinkat,
  /** @export */
  __syscall_renameat: ___syscall_renameat,
  /** @export */
  __syscall_stat64: ___syscall_stat64,
  /** @export */
  __syscall_statfs64: ___syscall_statfs64,
  /** @export */
  __syscall_unlinkat: ___syscall_unlinkat,
  /** @export */
  _abort_js: __abort_js,
  /** @export */
  _emscripten_runtime_keepalive_clear: __emscripten_runtime_keepalive_clear,
  /** @export */
  _gmtime_js: __gmtime_js,
  /** @export */
  _localtime_js: __localtime_js,
  /** @export */
  _tzset_js: __tzset_js,
  /** @export */
  alBufferData: _alBufferData,
  /** @export */
  alBufferiv: _alBufferiv,
  /** @export */
  alDeleteBuffers: _alDeleteBuffers,
  /** @export */
  alDeleteSources: _alDeleteSources,
  /** @export */
  alDistanceModel: _alDistanceModel,
  /** @export */
  alGenBuffers: _alGenBuffers,
  /** @export */
  alGenSources: _alGenSources,
  /** @export */
  alGetSourcei: _alGetSourcei,
  /** @export */
  alGetString: _alGetString,
  /** @export */
  alIsBuffer: _alIsBuffer,
  /** @export */
  alListener3f: _alListener3f,
  /** @export */
  alListenerf: _alListenerf,
  /** @export */
  alListenerfv: _alListenerfv,
  /** @export */
  alSource3f: _alSource3f,
  /** @export */
  alSource3i: _alSource3i,
  /** @export */
  alSourcePause: _alSourcePause,
  /** @export */
  alSourcePlay: _alSourcePlay,
  /** @export */
  alSourceQueueBuffers: _alSourceQueueBuffers,
  /** @export */
  alSourceStop: _alSourceStop,
  /** @export */
  alSourceUnqueueBuffers: _alSourceUnqueueBuffers,
  /** @export */
  alSourcef: _alSourcef,
  /** @export */
  alSourcei: _alSourcei,
  /** @export */
  alcCloseDevice: _alcCloseDevice,
  /** @export */
  alcCreateContext: _alcCreateContext,
  /** @export */
  alcDestroyContext: _alcDestroyContext,
  /** @export */
  alcIsExtensionPresent: _alcIsExtensionPresent,
  /** @export */
  alcMakeContextCurrent: _alcMakeContextCurrent,
  /** @export */
  alcOpenDevice: _alcOpenDevice,
  /** @export */
  alcSuspendContext: _alcSuspendContext,
  /** @export */
  clock_time_get: _clock_time_get,
  /** @export */
  emscripten_asm_const_int: _emscripten_asm_const_int,
  /** @export */
  emscripten_cancel_main_loop: _emscripten_cancel_main_loop,
  /** @export */
  emscripten_date_now: _emscripten_date_now,
  /** @export */
  emscripten_get_gamepad_status: _emscripten_get_gamepad_status,
  /** @export */
  emscripten_get_now: _emscripten_get_now,
  /** @export */
  emscripten_get_num_gamepads: _emscripten_get_num_gamepads,
  /** @export */
  emscripten_glActiveTexture: _emscripten_glActiveTexture,
  /** @export */
  emscripten_glAttachShader: _emscripten_glAttachShader,
  /** @export */
  emscripten_glBeginQuery: _emscripten_glBeginQuery,
  /** @export */
  emscripten_glBeginQueryEXT: _emscripten_glBeginQueryEXT,
  /** @export */
  emscripten_glBeginTransformFeedback: _emscripten_glBeginTransformFeedback,
  /** @export */
  emscripten_glBindAttribLocation: _emscripten_glBindAttribLocation,
  /** @export */
  emscripten_glBindBuffer: _emscripten_glBindBuffer,
  /** @export */
  emscripten_glBindBufferBase: _emscripten_glBindBufferBase,
  /** @export */
  emscripten_glBindBufferRange: _emscripten_glBindBufferRange,
  /** @export */
  emscripten_glBindFramebuffer: _emscripten_glBindFramebuffer,
  /** @export */
  emscripten_glBindRenderbuffer: _emscripten_glBindRenderbuffer,
  /** @export */
  emscripten_glBindSampler: _emscripten_glBindSampler,
  /** @export */
  emscripten_glBindTexture: _emscripten_glBindTexture,
  /** @export */
  emscripten_glBindTransformFeedback: _emscripten_glBindTransformFeedback,
  /** @export */
  emscripten_glBindVertexArray: _emscripten_glBindVertexArray,
  /** @export */
  emscripten_glBindVertexArrayOES: _emscripten_glBindVertexArrayOES,
  /** @export */
  emscripten_glBlendColor: _emscripten_glBlendColor,
  /** @export */
  emscripten_glBlendEquation: _emscripten_glBlendEquation,
  /** @export */
  emscripten_glBlendEquationSeparate: _emscripten_glBlendEquationSeparate,
  /** @export */
  emscripten_glBlendFunc: _emscripten_glBlendFunc,
  /** @export */
  emscripten_glBlendFuncSeparate: _emscripten_glBlendFuncSeparate,
  /** @export */
  emscripten_glBlitFramebuffer: _emscripten_glBlitFramebuffer,
  /** @export */
  emscripten_glBufferData: _emscripten_glBufferData,
  /** @export */
  emscripten_glBufferSubData: _emscripten_glBufferSubData,
  /** @export */
  emscripten_glCheckFramebufferStatus: _emscripten_glCheckFramebufferStatus,
  /** @export */
  emscripten_glClear: _emscripten_glClear,
  /** @export */
  emscripten_glClearBufferfi: _emscripten_glClearBufferfi,
  /** @export */
  emscripten_glClearBufferfv: _emscripten_glClearBufferfv,
  /** @export */
  emscripten_glClearBufferiv: _emscripten_glClearBufferiv,
  /** @export */
  emscripten_glClearBufferuiv: _emscripten_glClearBufferuiv,
  /** @export */
  emscripten_glClearColor: _emscripten_glClearColor,
  /** @export */
  emscripten_glClearDepthf: _emscripten_glClearDepthf,
  /** @export */
  emscripten_glClearStencil: _emscripten_glClearStencil,
  /** @export */
  emscripten_glClientWaitSync: _emscripten_glClientWaitSync,
  /** @export */
  emscripten_glClipControlEXT: _emscripten_glClipControlEXT,
  /** @export */
  emscripten_glColorMask: _emscripten_glColorMask,
  /** @export */
  emscripten_glCompileShader: _emscripten_glCompileShader,
  /** @export */
  emscripten_glCompressedTexImage2D: _emscripten_glCompressedTexImage2D,
  /** @export */
  emscripten_glCompressedTexImage3D: _emscripten_glCompressedTexImage3D,
  /** @export */
  emscripten_glCompressedTexSubImage2D: _emscripten_glCompressedTexSubImage2D,
  /** @export */
  emscripten_glCompressedTexSubImage3D: _emscripten_glCompressedTexSubImage3D,
  /** @export */
  emscripten_glCopyBufferSubData: _emscripten_glCopyBufferSubData,
  /** @export */
  emscripten_glCopyTexImage2D: _emscripten_glCopyTexImage2D,
  /** @export */
  emscripten_glCopyTexSubImage2D: _emscripten_glCopyTexSubImage2D,
  /** @export */
  emscripten_glCopyTexSubImage3D: _emscripten_glCopyTexSubImage3D,
  /** @export */
  emscripten_glCreateProgram: _emscripten_glCreateProgram,
  /** @export */
  emscripten_glCreateShader: _emscripten_glCreateShader,
  /** @export */
  emscripten_glCullFace: _emscripten_glCullFace,
  /** @export */
  emscripten_glDeleteBuffers: _emscripten_glDeleteBuffers,
  /** @export */
  emscripten_glDeleteFramebuffers: _emscripten_glDeleteFramebuffers,
  /** @export */
  emscripten_glDeleteProgram: _emscripten_glDeleteProgram,
  /** @export */
  emscripten_glDeleteQueries: _emscripten_glDeleteQueries,
  /** @export */
  emscripten_glDeleteQueriesEXT: _emscripten_glDeleteQueriesEXT,
  /** @export */
  emscripten_glDeleteRenderbuffers: _emscripten_glDeleteRenderbuffers,
  /** @export */
  emscripten_glDeleteSamplers: _emscripten_glDeleteSamplers,
  /** @export */
  emscripten_glDeleteShader: _emscripten_glDeleteShader,
  /** @export */
  emscripten_glDeleteSync: _emscripten_glDeleteSync,
  /** @export */
  emscripten_glDeleteTextures: _emscripten_glDeleteTextures,
  /** @export */
  emscripten_glDeleteTransformFeedbacks: _emscripten_glDeleteTransformFeedbacks,
  /** @export */
  emscripten_glDeleteVertexArrays: _emscripten_glDeleteVertexArrays,
  /** @export */
  emscripten_glDeleteVertexArraysOES: _emscripten_glDeleteVertexArraysOES,
  /** @export */
  emscripten_glDepthFunc: _emscripten_glDepthFunc,
  /** @export */
  emscripten_glDepthMask: _emscripten_glDepthMask,
  /** @export */
  emscripten_glDepthRangef: _emscripten_glDepthRangef,
  /** @export */
  emscripten_glDetachShader: _emscripten_glDetachShader,
  /** @export */
  emscripten_glDisable: _emscripten_glDisable,
  /** @export */
  emscripten_glDisableVertexAttribArray: _emscripten_glDisableVertexAttribArray,
  /** @export */
  emscripten_glDrawArrays: _emscripten_glDrawArrays,
  /** @export */
  emscripten_glDrawArraysInstanced: _emscripten_glDrawArraysInstanced,
  /** @export */
  emscripten_glDrawArraysInstancedANGLE: _emscripten_glDrawArraysInstancedANGLE,
  /** @export */
  emscripten_glDrawArraysInstancedARB: _emscripten_glDrawArraysInstancedARB,
  /** @export */
  emscripten_glDrawArraysInstancedEXT: _emscripten_glDrawArraysInstancedEXT,
  /** @export */
  emscripten_glDrawArraysInstancedNV: _emscripten_glDrawArraysInstancedNV,
  /** @export */
  emscripten_glDrawBuffers: _emscripten_glDrawBuffers,
  /** @export */
  emscripten_glDrawBuffersEXT: _emscripten_glDrawBuffersEXT,
  /** @export */
  emscripten_glDrawBuffersWEBGL: _emscripten_glDrawBuffersWEBGL,
  /** @export */
  emscripten_glDrawElements: _emscripten_glDrawElements,
  /** @export */
  emscripten_glDrawElementsInstanced: _emscripten_glDrawElementsInstanced,
  /** @export */
  emscripten_glDrawElementsInstancedANGLE: _emscripten_glDrawElementsInstancedANGLE,
  /** @export */
  emscripten_glDrawElementsInstancedARB: _emscripten_glDrawElementsInstancedARB,
  /** @export */
  emscripten_glDrawElementsInstancedEXT: _emscripten_glDrawElementsInstancedEXT,
  /** @export */
  emscripten_glDrawElementsInstancedNV: _emscripten_glDrawElementsInstancedNV,
  /** @export */
  emscripten_glDrawRangeElements: _emscripten_glDrawRangeElements,
  /** @export */
  emscripten_glEnable: _emscripten_glEnable,
  /** @export */
  emscripten_glEnableVertexAttribArray: _emscripten_glEnableVertexAttribArray,
  /** @export */
  emscripten_glEndQuery: _emscripten_glEndQuery,
  /** @export */
  emscripten_glEndQueryEXT: _emscripten_glEndQueryEXT,
  /** @export */
  emscripten_glEndTransformFeedback: _emscripten_glEndTransformFeedback,
  /** @export */
  emscripten_glFenceSync: _emscripten_glFenceSync,
  /** @export */
  emscripten_glFinish: _emscripten_glFinish,
  /** @export */
  emscripten_glFlush: _emscripten_glFlush,
  /** @export */
  emscripten_glFlushMappedBufferRange: _emscripten_glFlushMappedBufferRange,
  /** @export */
  emscripten_glFramebufferRenderbuffer: _emscripten_glFramebufferRenderbuffer,
  /** @export */
  emscripten_glFramebufferTexture2D: _emscripten_glFramebufferTexture2D,
  /** @export */
  emscripten_glFramebufferTextureLayer: _emscripten_glFramebufferTextureLayer,
  /** @export */
  emscripten_glFrontFace: _emscripten_glFrontFace,
  /** @export */
  emscripten_glGenBuffers: _emscripten_glGenBuffers,
  /** @export */
  emscripten_glGenFramebuffers: _emscripten_glGenFramebuffers,
  /** @export */
  emscripten_glGenQueries: _emscripten_glGenQueries,
  /** @export */
  emscripten_glGenQueriesEXT: _emscripten_glGenQueriesEXT,
  /** @export */
  emscripten_glGenRenderbuffers: _emscripten_glGenRenderbuffers,
  /** @export */
  emscripten_glGenSamplers: _emscripten_glGenSamplers,
  /** @export */
  emscripten_glGenTextures: _emscripten_glGenTextures,
  /** @export */
  emscripten_glGenTransformFeedbacks: _emscripten_glGenTransformFeedbacks,
  /** @export */
  emscripten_glGenVertexArrays: _emscripten_glGenVertexArrays,
  /** @export */
  emscripten_glGenVertexArraysOES: _emscripten_glGenVertexArraysOES,
  /** @export */
  emscripten_glGenerateMipmap: _emscripten_glGenerateMipmap,
  /** @export */
  emscripten_glGetActiveAttrib: _emscripten_glGetActiveAttrib,
  /** @export */
  emscripten_glGetActiveUniform: _emscripten_glGetActiveUniform,
  /** @export */
  emscripten_glGetActiveUniformBlockName: _emscripten_glGetActiveUniformBlockName,
  /** @export */
  emscripten_glGetActiveUniformBlockiv: _emscripten_glGetActiveUniformBlockiv,
  /** @export */
  emscripten_glGetActiveUniformsiv: _emscripten_glGetActiveUniformsiv,
  /** @export */
  emscripten_glGetAttachedShaders: _emscripten_glGetAttachedShaders,
  /** @export */
  emscripten_glGetAttribLocation: _emscripten_glGetAttribLocation,
  /** @export */
  emscripten_glGetBooleanv: _emscripten_glGetBooleanv,
  /** @export */
  emscripten_glGetBufferParameteri64v: _emscripten_glGetBufferParameteri64v,
  /** @export */
  emscripten_glGetBufferParameteriv: _emscripten_glGetBufferParameteriv,
  /** @export */
  emscripten_glGetBufferPointerv: _emscripten_glGetBufferPointerv,
  /** @export */
  emscripten_glGetError: _emscripten_glGetError,
  /** @export */
  emscripten_glGetFloatv: _emscripten_glGetFloatv,
  /** @export */
  emscripten_glGetFragDataLocation: _emscripten_glGetFragDataLocation,
  /** @export */
  emscripten_glGetFramebufferAttachmentParameteriv: _emscripten_glGetFramebufferAttachmentParameteriv,
  /** @export */
  emscripten_glGetInteger64i_v: _emscripten_glGetInteger64i_v,
  /** @export */
  emscripten_glGetInteger64v: _emscripten_glGetInteger64v,
  /** @export */
  emscripten_glGetIntegeri_v: _emscripten_glGetIntegeri_v,
  /** @export */
  emscripten_glGetIntegerv: _emscripten_glGetIntegerv,
  /** @export */
  emscripten_glGetInternalformativ: _emscripten_glGetInternalformativ,
  /** @export */
  emscripten_glGetProgramBinary: _emscripten_glGetProgramBinary,
  /** @export */
  emscripten_glGetProgramInfoLog: _emscripten_glGetProgramInfoLog,
  /** @export */
  emscripten_glGetProgramiv: _emscripten_glGetProgramiv,
  /** @export */
  emscripten_glGetQueryObjecti64vEXT: _emscripten_glGetQueryObjecti64vEXT,
  /** @export */
  emscripten_glGetQueryObjectivEXT: _emscripten_glGetQueryObjectivEXT,
  /** @export */
  emscripten_glGetQueryObjectui64vEXT: _emscripten_glGetQueryObjectui64vEXT,
  /** @export */
  emscripten_glGetQueryObjectuiv: _emscripten_glGetQueryObjectuiv,
  /** @export */
  emscripten_glGetQueryObjectuivEXT: _emscripten_glGetQueryObjectuivEXT,
  /** @export */
  emscripten_glGetQueryiv: _emscripten_glGetQueryiv,
  /** @export */
  emscripten_glGetQueryivEXT: _emscripten_glGetQueryivEXT,
  /** @export */
  emscripten_glGetRenderbufferParameteriv: _emscripten_glGetRenderbufferParameteriv,
  /** @export */
  emscripten_glGetSamplerParameterfv: _emscripten_glGetSamplerParameterfv,
  /** @export */
  emscripten_glGetSamplerParameteriv: _emscripten_glGetSamplerParameteriv,
  /** @export */
  emscripten_glGetShaderInfoLog: _emscripten_glGetShaderInfoLog,
  /** @export */
  emscripten_glGetShaderPrecisionFormat: _emscripten_glGetShaderPrecisionFormat,
  /** @export */
  emscripten_glGetShaderSource: _emscripten_glGetShaderSource,
  /** @export */
  emscripten_glGetShaderiv: _emscripten_glGetShaderiv,
  /** @export */
  emscripten_glGetString: _emscripten_glGetString,
  /** @export */
  emscripten_glGetStringi: _emscripten_glGetStringi,
  /** @export */
  emscripten_glGetSynciv: _emscripten_glGetSynciv,
  /** @export */
  emscripten_glGetTexParameterfv: _emscripten_glGetTexParameterfv,
  /** @export */
  emscripten_glGetTexParameteriv: _emscripten_glGetTexParameteriv,
  /** @export */
  emscripten_glGetTransformFeedbackVarying: _emscripten_glGetTransformFeedbackVarying,
  /** @export */
  emscripten_glGetUniformBlockIndex: _emscripten_glGetUniformBlockIndex,
  /** @export */
  emscripten_glGetUniformIndices: _emscripten_glGetUniformIndices,
  /** @export */
  emscripten_glGetUniformLocation: _emscripten_glGetUniformLocation,
  /** @export */
  emscripten_glGetUniformfv: _emscripten_glGetUniformfv,
  /** @export */
  emscripten_glGetUniformiv: _emscripten_glGetUniformiv,
  /** @export */
  emscripten_glGetUniformuiv: _emscripten_glGetUniformuiv,
  /** @export */
  emscripten_glGetVertexAttribIiv: _emscripten_glGetVertexAttribIiv,
  /** @export */
  emscripten_glGetVertexAttribIuiv: _emscripten_glGetVertexAttribIuiv,
  /** @export */
  emscripten_glGetVertexAttribPointerv: _emscripten_glGetVertexAttribPointerv,
  /** @export */
  emscripten_glGetVertexAttribfv: _emscripten_glGetVertexAttribfv,
  /** @export */
  emscripten_glGetVertexAttribiv: _emscripten_glGetVertexAttribiv,
  /** @export */
  emscripten_glHint: _emscripten_glHint,
  /** @export */
  emscripten_glInvalidateFramebuffer: _emscripten_glInvalidateFramebuffer,
  /** @export */
  emscripten_glInvalidateSubFramebuffer: _emscripten_glInvalidateSubFramebuffer,
  /** @export */
  emscripten_glIsBuffer: _emscripten_glIsBuffer,
  /** @export */
  emscripten_glIsEnabled: _emscripten_glIsEnabled,
  /** @export */
  emscripten_glIsFramebuffer: _emscripten_glIsFramebuffer,
  /** @export */
  emscripten_glIsProgram: _emscripten_glIsProgram,
  /** @export */
  emscripten_glIsQuery: _emscripten_glIsQuery,
  /** @export */
  emscripten_glIsQueryEXT: _emscripten_glIsQueryEXT,
  /** @export */
  emscripten_glIsRenderbuffer: _emscripten_glIsRenderbuffer,
  /** @export */
  emscripten_glIsSampler: _emscripten_glIsSampler,
  /** @export */
  emscripten_glIsShader: _emscripten_glIsShader,
  /** @export */
  emscripten_glIsSync: _emscripten_glIsSync,
  /** @export */
  emscripten_glIsTexture: _emscripten_glIsTexture,
  /** @export */
  emscripten_glIsTransformFeedback: _emscripten_glIsTransformFeedback,
  /** @export */
  emscripten_glIsVertexArray: _emscripten_glIsVertexArray,
  /** @export */
  emscripten_glIsVertexArrayOES: _emscripten_glIsVertexArrayOES,
  /** @export */
  emscripten_glLineWidth: _emscripten_glLineWidth,
  /** @export */
  emscripten_glLinkProgram: _emscripten_glLinkProgram,
  /** @export */
  emscripten_glMapBufferRange: _emscripten_glMapBufferRange,
  /** @export */
  emscripten_glPauseTransformFeedback: _emscripten_glPauseTransformFeedback,
  /** @export */
  emscripten_glPixelStorei: _emscripten_glPixelStorei,
  /** @export */
  emscripten_glPolygonModeWEBGL: _emscripten_glPolygonModeWEBGL,
  /** @export */
  emscripten_glPolygonOffset: _emscripten_glPolygonOffset,
  /** @export */
  emscripten_glPolygonOffsetClampEXT: _emscripten_glPolygonOffsetClampEXT,
  /** @export */
  emscripten_glProgramBinary: _emscripten_glProgramBinary,
  /** @export */
  emscripten_glProgramParameteri: _emscripten_glProgramParameteri,
  /** @export */
  emscripten_glQueryCounterEXT: _emscripten_glQueryCounterEXT,
  /** @export */
  emscripten_glReadBuffer: _emscripten_glReadBuffer,
  /** @export */
  emscripten_glReadPixels: _emscripten_glReadPixels,
  /** @export */
  emscripten_glReleaseShaderCompiler: _emscripten_glReleaseShaderCompiler,
  /** @export */
  emscripten_glRenderbufferStorage: _emscripten_glRenderbufferStorage,
  /** @export */
  emscripten_glRenderbufferStorageMultisample: _emscripten_glRenderbufferStorageMultisample,
  /** @export */
  emscripten_glResumeTransformFeedback: _emscripten_glResumeTransformFeedback,
  /** @export */
  emscripten_glSampleCoverage: _emscripten_glSampleCoverage,
  /** @export */
  emscripten_glSamplerParameterf: _emscripten_glSamplerParameterf,
  /** @export */
  emscripten_glSamplerParameterfv: _emscripten_glSamplerParameterfv,
  /** @export */
  emscripten_glSamplerParameteri: _emscripten_glSamplerParameteri,
  /** @export */
  emscripten_glSamplerParameteriv: _emscripten_glSamplerParameteriv,
  /** @export */
  emscripten_glScissor: _emscripten_glScissor,
  /** @export */
  emscripten_glShaderBinary: _emscripten_glShaderBinary,
  /** @export */
  emscripten_glShaderSource: _emscripten_glShaderSource,
  /** @export */
  emscripten_glStencilFunc: _emscripten_glStencilFunc,
  /** @export */
  emscripten_glStencilFuncSeparate: _emscripten_glStencilFuncSeparate,
  /** @export */
  emscripten_glStencilMask: _emscripten_glStencilMask,
  /** @export */
  emscripten_glStencilMaskSeparate: _emscripten_glStencilMaskSeparate,
  /** @export */
  emscripten_glStencilOp: _emscripten_glStencilOp,
  /** @export */
  emscripten_glStencilOpSeparate: _emscripten_glStencilOpSeparate,
  /** @export */
  emscripten_glTexImage2D: _emscripten_glTexImage2D,
  /** @export */
  emscripten_glTexImage3D: _emscripten_glTexImage3D,
  /** @export */
  emscripten_glTexParameterf: _emscripten_glTexParameterf,
  /** @export */
  emscripten_glTexParameterfv: _emscripten_glTexParameterfv,
  /** @export */
  emscripten_glTexParameteri: _emscripten_glTexParameteri,
  /** @export */
  emscripten_glTexParameteriv: _emscripten_glTexParameteriv,
  /** @export */
  emscripten_glTexStorage2D: _emscripten_glTexStorage2D,
  /** @export */
  emscripten_glTexStorage3D: _emscripten_glTexStorage3D,
  /** @export */
  emscripten_glTexSubImage2D: _emscripten_glTexSubImage2D,
  /** @export */
  emscripten_glTexSubImage3D: _emscripten_glTexSubImage3D,
  /** @export */
  emscripten_glTransformFeedbackVaryings: _emscripten_glTransformFeedbackVaryings,
  /** @export */
  emscripten_glUniform1f: _emscripten_glUniform1f,
  /** @export */
  emscripten_glUniform1fv: _emscripten_glUniform1fv,
  /** @export */
  emscripten_glUniform1i: _emscripten_glUniform1i,
  /** @export */
  emscripten_glUniform1iv: _emscripten_glUniform1iv,
  /** @export */
  emscripten_glUniform1ui: _emscripten_glUniform1ui,
  /** @export */
  emscripten_glUniform1uiv: _emscripten_glUniform1uiv,
  /** @export */
  emscripten_glUniform2f: _emscripten_glUniform2f,
  /** @export */
  emscripten_glUniform2fv: _emscripten_glUniform2fv,
  /** @export */
  emscripten_glUniform2i: _emscripten_glUniform2i,
  /** @export */
  emscripten_glUniform2iv: _emscripten_glUniform2iv,
  /** @export */
  emscripten_glUniform2ui: _emscripten_glUniform2ui,
  /** @export */
  emscripten_glUniform2uiv: _emscripten_glUniform2uiv,
  /** @export */
  emscripten_glUniform3f: _emscripten_glUniform3f,
  /** @export */
  emscripten_glUniform3fv: _emscripten_glUniform3fv,
  /** @export */
  emscripten_glUniform3i: _emscripten_glUniform3i,
  /** @export */
  emscripten_glUniform3iv: _emscripten_glUniform3iv,
  /** @export */
  emscripten_glUniform3ui: _emscripten_glUniform3ui,
  /** @export */
  emscripten_glUniform3uiv: _emscripten_glUniform3uiv,
  /** @export */
  emscripten_glUniform4f: _emscripten_glUniform4f,
  /** @export */
  emscripten_glUniform4fv: _emscripten_glUniform4fv,
  /** @export */
  emscripten_glUniform4i: _emscripten_glUniform4i,
  /** @export */
  emscripten_glUniform4iv: _emscripten_glUniform4iv,
  /** @export */
  emscripten_glUniform4ui: _emscripten_glUniform4ui,
  /** @export */
  emscripten_glUniform4uiv: _emscripten_glUniform4uiv,
  /** @export */
  emscripten_glUniformBlockBinding: _emscripten_glUniformBlockBinding,
  /** @export */
  emscripten_glUniformMatrix2fv: _emscripten_glUniformMatrix2fv,
  /** @export */
  emscripten_glUniformMatrix2x3fv: _emscripten_glUniformMatrix2x3fv,
  /** @export */
  emscripten_glUniformMatrix2x4fv: _emscripten_glUniformMatrix2x4fv,
  /** @export */
  emscripten_glUniformMatrix3fv: _emscripten_glUniformMatrix3fv,
  /** @export */
  emscripten_glUniformMatrix3x2fv: _emscripten_glUniformMatrix3x2fv,
  /** @export */
  emscripten_glUniformMatrix3x4fv: _emscripten_glUniformMatrix3x4fv,
  /** @export */
  emscripten_glUniformMatrix4fv: _emscripten_glUniformMatrix4fv,
  /** @export */
  emscripten_glUniformMatrix4x2fv: _emscripten_glUniformMatrix4x2fv,
  /** @export */
  emscripten_glUniformMatrix4x3fv: _emscripten_glUniformMatrix4x3fv,
  /** @export */
  emscripten_glUnmapBuffer: _emscripten_glUnmapBuffer,
  /** @export */
  emscripten_glUseProgram: _emscripten_glUseProgram,
  /** @export */
  emscripten_glValidateProgram: _emscripten_glValidateProgram,
  /** @export */
  emscripten_glVertexAttrib1f: _emscripten_glVertexAttrib1f,
  /** @export */
  emscripten_glVertexAttrib1fv: _emscripten_glVertexAttrib1fv,
  /** @export */
  emscripten_glVertexAttrib2f: _emscripten_glVertexAttrib2f,
  /** @export */
  emscripten_glVertexAttrib2fv: _emscripten_glVertexAttrib2fv,
  /** @export */
  emscripten_glVertexAttrib3f: _emscripten_glVertexAttrib3f,
  /** @export */
  emscripten_glVertexAttrib3fv: _emscripten_glVertexAttrib3fv,
  /** @export */
  emscripten_glVertexAttrib4f: _emscripten_glVertexAttrib4f,
  /** @export */
  emscripten_glVertexAttrib4fv: _emscripten_glVertexAttrib4fv,
  /** @export */
  emscripten_glVertexAttribDivisor: _emscripten_glVertexAttribDivisor,
  /** @export */
  emscripten_glVertexAttribDivisorANGLE: _emscripten_glVertexAttribDivisorANGLE,
  /** @export */
  emscripten_glVertexAttribDivisorARB: _emscripten_glVertexAttribDivisorARB,
  /** @export */
  emscripten_glVertexAttribDivisorEXT: _emscripten_glVertexAttribDivisorEXT,
  /** @export */
  emscripten_glVertexAttribDivisorNV: _emscripten_glVertexAttribDivisorNV,
  /** @export */
  emscripten_glVertexAttribI4i: _emscripten_glVertexAttribI4i,
  /** @export */
  emscripten_glVertexAttribI4iv: _emscripten_glVertexAttribI4iv,
  /** @export */
  emscripten_glVertexAttribI4ui: _emscripten_glVertexAttribI4ui,
  /** @export */
  emscripten_glVertexAttribI4uiv: _emscripten_glVertexAttribI4uiv,
  /** @export */
  emscripten_glVertexAttribIPointer: _emscripten_glVertexAttribIPointer,
  /** @export */
  emscripten_glVertexAttribPointer: _emscripten_glVertexAttribPointer,
  /** @export */
  emscripten_glViewport: _emscripten_glViewport,
  /** @export */
  emscripten_glWaitSync: _emscripten_glWaitSync,
  /** @export */
  emscripten_resize_heap: _emscripten_resize_heap,
  /** @export */
  emscripten_sample_gamepad_data: _emscripten_sample_gamepad_data,
  /** @export */
  emscripten_set_canvas_element_size: _emscripten_set_canvas_element_size,
  /** @export */
  emscripten_set_main_loop: _emscripten_set_main_loop,
  /** @export */
  emscripten_webgl_enable_extension: _emscripten_webgl_enable_extension,
  /** @export */
  emscripten_webgl_get_current_context: _emscripten_webgl_get_current_context,
  /** @export */
  environ_get: _environ_get,
  /** @export */
  environ_sizes_get: _environ_sizes_get,
  /** @export */
  exit: _exit,
  /** @export */
  fd_close: _fd_close,
  /** @export */
  fd_read: _fd_read,
  /** @export */
  fd_seek: _fd_seek,
  /** @export */
  fd_write: _fd_write,
  /** @export */
  glfwCreateWindow: _glfwCreateWindow,
  /** @export */
  glfwDestroyWindow: _glfwDestroyWindow,
  /** @export */
  glfwGetCursorPos: _glfwGetCursorPos,
  /** @export */
  glfwGetFramebufferSize: _glfwGetFramebufferSize,
  /** @export */
  glfwGetInputMode: _glfwGetInputMode,
  /** @export */
  glfwGetJoystickButtons: _glfwGetJoystickButtons,
  /** @export */
  glfwGetJoystickName: _glfwGetJoystickName,
  /** @export */
  glfwGetMonitorName: _glfwGetMonitorName,
  /** @export */
  glfwGetMonitors: _glfwGetMonitors,
  /** @export */
  glfwGetMouseButton: _glfwGetMouseButton,
  /** @export */
  glfwGetVideoModes: _glfwGetVideoModes,
  /** @export */
  glfwGetWindowSize: _glfwGetWindowSize,
  /** @export */
  glfwInit: _glfwInit,
  /** @export */
  glfwJoystickPresent: _glfwJoystickPresent,
  /** @export */
  glfwMakeContextCurrent: _glfwMakeContextCurrent,
  /** @export */
  glfwPollEvents: _glfwPollEvents,
  /** @export */
  glfwSetCursorEnterCallback: _glfwSetCursorEnterCallback,
  /** @export */
  glfwSetCursorPos: _glfwSetCursorPos,
  /** @export */
  glfwSetCursorPosCallback: _glfwSetCursorPosCallback,
  /** @export */
  glfwSetErrorCallback: _glfwSetErrorCallback,
  /** @export */
  glfwSetFramebufferSizeCallback: _glfwSetFramebufferSizeCallback,
  /** @export */
  glfwSetInputMode: _glfwSetInputMode,
  /** @export */
  glfwSetJoystickCallback: _glfwSetJoystickCallback,
  /** @export */
  glfwSetKeyCallback: _glfwSetKeyCallback,
  /** @export */
  glfwSetScrollCallback: _glfwSetScrollCallback,
  /** @export */
  glfwSetWindowFocusCallback: _glfwSetWindowFocusCallback,
  /** @export */
  glfwSetWindowIconifyCallback: _glfwSetWindowIconifyCallback,
  /** @export */
  glfwSetWindowSize: _glfwSetWindowSize,
  /** @export */
  glfwSwapBuffers: _glfwSwapBuffers,
  /** @export */
  glfwSwapInterval: _glfwSwapInterval,
  /** @export */
  glfwTerminate: _glfwTerminate,
  /** @export */
  glfwWindowHint: _glfwWindowHint,
  /** @export */
  glfwWindowShouldClose: _glfwWindowShouldClose,
  /** @export */
  od_present,
  /** @export */
  od_sfx_ready,
  /** @export */
  proc_exit: _proc_exit
};


// include: postamble.js
// === Auto-generated postamble setup entry stuff ===

async function callMain(args = []) {

  var entryFunction = _main;

  args.unshift(thisProgram);

  var argc = args.length;
  var argv = stackAlloc((argc + 1) * 4);
  var argv_ptr = argv;
  for (var arg of args) {
    HEAPU32[((argv_ptr)>>2)] = stringToUTF8OnStack(arg);
    argv_ptr += 4;
  }
  HEAPU32[((argv_ptr)>>2)] = 0;

  try {

    var ret = entryFunction(argc, argv);

    // The current spec of JSPI returns a promise only if the function suspends
    // and a plain value otherwise. This will likely change:
    // https://github.com/WebAssembly/js-promise-integration/issues/11
    ret = await ret;
    // if we're not running an evented main loop, it's time to exit
    exitJS(ret, /* implicit = */ true);
    return ret;
  } catch (e) {
    return handleException(e);
  }
}

async function run(args = programArgs) {

  preRun();

  if (runDependencies) {
    await resolveRunDependencies();
  }

  var setStatus = Module['setStatus'];
  if (setStatus) {
    setStatus('Running...');
    // Yield to the event loop to allow the browser to paint "Running..."
    await new Promise((resolve) => setTimeout(resolve, 1));
    // Then we want to clear the status text, but only after the rest of this function runs.
    setTimeout(setStatus, 1, '');
  }

  if (ABORT) return;

  initRuntime();

  // No ATMAINS hooks

  Module['onRuntimeInitialized']?.();

  var noInitialRun = Module['noInitialRun'] || false;
  if (!noInitialRun) await callMain(args);

  postRun();
}

var wasmExports;

// With async instantation wasmExports is assigned asynchronously when the
// instance is received.
createWasm().then(() => run());

// end include: postamble.js

