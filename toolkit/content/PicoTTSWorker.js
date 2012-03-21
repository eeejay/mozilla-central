importScripts('resource:///modules/osfile.jsm');

var PicoWrapper = {
  PICO_MEM_SIZE: 2500000,
  PICO_VOICE_NAME: 'PicoVoice',

  MemArea: null,
  System: null,
  Engine: null,
  TaResource: null,
  SgResource: null,
  picoSgResource: null,
  TaResourceName: null,

  ttslib: null,

  PICO_RESET_SOFT: 0x10,

  PICO_DATA_PCM_16BIT: 1,

  SystemType: ctypes.StructType('pico_system').ptr,
  ResourceType: ctypes.StructType('pico_resource').ptr,
  EngineType: ctypes.StructType('pico_engine').ptr,
  RetstringType: ctypes.ArrayType(ctypes.char),

  load: function load() {
    if (this.ttslib)
      return;

    this.ttslib = ctypes.open('libttspico.so');

    this.initialize = this.ttslib
      .declare('pico_initialize',
               ctypes.default_abi,
               ctypes.int32_t,
               ctypes.voidptr_t,
               ctypes.uint32_t,
               this.SystemType.ptr);

    this.terminate = this.ttslib
      .declare('pico_terminate',
               ctypes.default_abi,
               ctypes.int32_t,
               this.SystemType.ptr);

    this.loadResource = this.ttslib
      .declare('pico_loadResource',
               ctypes.default_abi,
               ctypes.int32_t,
               this.SystemType,
               ctypes.char.ptr,
               this.ResourceType.ptr);

    this.createVoiceDefinition = this.ttslib
      .declare('pico_createVoiceDefinition',
               ctypes.default_abi,
               ctypes.int32_t,
               this.SystemType,
               ctypes.char.ptr);

    this.addResourceToVoiceDefinition = this.ttslib
      .declare('pico_addResourceToVoiceDefinition',
               ctypes.default_abi,
               ctypes.int32_t,
               this.SystemType,
               ctypes.char.ptr,
               ctypes.char.ptr);

    this.newEngine = this.ttslib
      .declare('pico_newEngine',
               ctypes.default_abi,
               ctypes.int32_t,
               this.SystemType,
               ctypes.char.ptr,
               this.EngineType.ptr);

    this.putTextUtf8 = this.ttslib
      .declare('pico_putTextUtf8',
               ctypes.default_abi,
               ctypes.int32_t,
               this.EngineType,
               ctypes.char.ptr,
               ctypes.int16_t,
               ctypes.int16_t.ptr);

    this.getData = this.ttslib
      .declare('pico_getData',
               ctypes.default_abi,
               ctypes.int32_t,
               this.EngineType,
               ctypes.voidptr_t,
               ctypes.int16_t,
               ctypes.int16_t.ptr,
               ctypes.int16_t.ptr);

    this.disposeEngine = this.ttslib
      .declare('pico_disposeEngine',
               ctypes.default_abi,
               ctypes.int32_t,
               this.SystemType,
               this.EngineType.ptr);

    this.releaseVoiceDefinition = this.ttslib
      .declare('pico_releaseVoiceDefinition',
               ctypes.default_abi,
               ctypes.int32_t,
               this.SystemType,
               ctypes.char.ptr);

    this.unloadResource = this.ttslib
      .declare('pico_unloadResource',
               ctypes.default_abi,
               ctypes.int32_t,
               this.SystemType,
               this.ResourceType.ptr);

    this.terminate = this.ttslib
      .declare('pico_terminate',
               ctypes.default_abi,
               ctypes.int32_t,
               this.SystemType.ptr);

    this.getSystemStatusMessage = this.ttslib
      .declare('pico_getSystemStatusMessage',
               ctypes.default_abi,
               ctypes.int32_t,
               this.SystemType,
               ctypes.int32_t,
               this.RetstringType);

    this.getResourceName = this.ttslib
      .declare('pico_getResourceName',
               ctypes.default_abi,
               ctypes.int32_t,
               this.SystemType,
               this.ResourceType,
               this.RetstringType);

    this.resetEngine = this.ttslib
      .declare('pico_resetEngine',
               ctypes.default_abi,
               ctypes.int32_t,
               this.EngineType,
               ctypes.int32_t);
  },

  init: function init() {
    if (!this.ttslib)
      this.load();

    let rv = 0;
    this.MemArea = new (ctypes.ArrayType(ctypes.voidptr_t))(this.PICO_MEM_SIZE);
    this.System = new this.SystemType(0);

    rv = this.initialize(this.MemArea, this.PICO_MEM_SIZE,
                         this.System.address());
    this.throwExceptionOnError('pico_initialize', rv);
  },

  loadEngine: function loadEngine(aLanguage, aTaFile, aSgFile) {
    dump('PicoTTS: loadEngine ' + aLanguage + '\n');

    this.TaResource = new this.ResourceType(0);
    this.SgResource = new this.ResourceType(0);
    this.Engine = new this.EngineType(0);

    let rv = this.loadResource(this.System, aTaFile, this.TaResource.address());
    this.throwExceptionOnError('pico_loadResource', rv);

    rv = this.loadResource(this.System, aSgFile, this.SgResource.address());
    this.throwExceptionOnError('pico_loadResource', rv);

    rv = this.createVoiceDefinition(this.System, this.PICO_VOICE_NAME);
    this.throwExceptionOnError('pico_createVoiceDefinition', rv);

    let resourceName = new this.RetstringType(200);
    rv = this.getResourceName(this.System, this.TaResource, resourceName);
    this.throwExceptionOnError('pico_getResourceName', rv);

    rv = this.addResourceToVoiceDefinition(this.System, this.PICO_VOICE_NAME,
                                           resourceName.readString());
    this.throwExceptionOnError('pico_addResourceToVoiceDefinition', rv);

    rv = this.getResourceName(this.System, this.SgResource, resourceName);
    this.throwExceptionOnError('pico_getResourceName', rv);

    rv = this.addResourceToVoiceDefinition(this.System, this.PICO_VOICE_NAME,
                                           resourceName.readString());
    this.throwExceptionOnError('pico_addResourceToVoiceDefinition', rv);

    rv = this.newEngine(this.System, this.PICO_VOICE_NAME,
                        this.Engine.address());
    this.throwExceptionOnError('pico_newEngine', rv);

    this.currentLanguage = aLanguage;
  },

  unloadEngine: function unloadEngine() {
    let rv = this.disposeEngine(this.System, this.Engine.address());
    this.throwExceptionOnError('pico_disposeEngine', rv);
    this.Engine = null;

    rv = this.releaseVoiceDefinition(this.System, this.PICO_VOICE_NAME);
    this.throwExceptionOnError('pico_releaseVoiceDefinition', rv);

    rv = this.unloadResource(this.System, this.SgResource.address());
    this.throwExceptionOnError('pico_unloadResource', rv);
    this.SgResource = null;

    rv = this.unloadResource(this.System, this.TaResource.address());
    this.throwExceptionOnError('pico_unloadResource', rv);
    this.TaResource = null;

    this.currentLanguage = null;
  },

  uninit: function uninit() {
    let rv = this.terminate(this.System.address());
    this.throwExceptionOnError('pico_terminate', rv);
    this.System = null;

    this.MemArea = null;
  },

  throwExceptionOnError: function(funcname, status) {
    if (status >= 0)
      return;

    let picoErrorMessage = new this.RetstringType(200);
    this.getSystemStatusMessage(this.System, status, picoErrorMessage);
    throw new Error(funcname + ': ' + picoErrorMessage.readString());
  }
};

var PicoTTSWorker = {
  init: function init(aLanguagePath) {
    try {
      PicoWrapper.init(aLanguagePath);
      this._voices = [];
      let taRe = /(.*)_ta.bin/;
      let sgRe = /(.*)_sg.bin/;
      let langRe = /(.*?)_.*.bin/;
      let iterator = new OS.File.DirectoryIterator(aLanguagePath);
      this._languages = {};
      try {
        for (let entry in iterator) {
          let match = langRe.exec(entry.name);
          if (!match)
            continue;
          let langName = match[1];
          let lang = this._languages[match[1]] || {};
          match = taRe.exec(entry.name);
          if (match)
            lang.taFile = aLanguagePath + '/' + entry.name;
          match = sgRe.exec(entry.name);
          if (match)
            lang.sgFile = aLanguagePath + '/' + entry.name;
          this._languages[langName] = lang;
        }
      } finally {
        iterator.close();
      }
    } catch (x) {
      dump('Pico TTS: Error initializing pico: ' + x + '\n');
      this._voices = [];
    }

    let voices = [];
    for (var langName in this._languages) {
      let lang = this._languages[langName];
      if (!lang.taFile || !lang.sgFile)
        continue;

      voices.push({voiceName: 'Pico ' + langName,
                   lang: langName,
                   engineId: 'PicoTTS'});
    }

    self.postMessage({type: 'voices', voices: voices});
  },

  synthesize: function synthesize(aUtterance, aOptions) {
    let utterance = aUtterance;

    if (aOptions.pitch != undefined) {
      let pitch = Math.round(
        (aOptions.pitch >= 1) ?
          Math.min(aOptions.pitch * 100, 200) :
          Math.max(aOptions.pitch * 50 + 50, 50));
      utterance = '<pitch level="' + pitch + '">' + utterance + '</pitch>';
    }

    if (aOptions.rate != undefined) {
      let speed = Math.round(
        Math.min(Math.max(aOptions.rate * 100, 20), 500));
      utterance = '<speed level="' + speed + '">' + utterance + '</speed>';
    }

    if (aOptions.volume != undefined) {
      let volume = Math.round(Math.min(100, aOptions.volume * 100));
      utterance = '<volume level="' + volume + '">' + utterance + '</volume>';
    }

    let lang = this._languages[aOptions.lang] || 'en-US';
    let langInfo = this._languages[lang];
    if (!PicoWrapper.Engine) {
      PicoWrapper.loadEngine(lang, langInfo.taFile, langInfo.sgFile);
    } else if (PicoWrapper.currentLanguage != lang) {
      PicoWrapper.unloadEngine();
      PicoWrapper.loadEngine(lang, langInfo.taFile, langInfo.sgFile);
    } else {
      this.resetEngine();
    }

    let text_remaining = utterance.length;
    let sampleRate = 0;
    let outbuf = new (ctypes.ArrayType(ctypes.uint8_t))(256);
    let gotBytes = ctypes.int16_t(0);

    function feedEngine() {
      if (text_remaining <= 0)
        return false;

      let str = ctypes.char.array()(
        utterance.slice(utterance.length - text_remaining));

      let sentBytes = ctypes.int16_t(0);
      let rv = PicoWrapper.putTextUtf8(PicoWrapper.Engine, str, str.length,
                                       sentBytes.address());
      PicoWrapper.throwExceptionOnError('putTextUtf8', rv);
      text_remaining -= sentBytes.value;

      return true;
    }

    function getAudio(aMilliseconds) {
      let samples = sampleRate * (aMilliseconds / 1000);
      let audioData = (samples) ? new Float32Array(samples) : null;
      let audioDataOffset = 0;

      function flushBuffer() {
        let sampleLength = gotBytes.value / 2;
        if (!sampleLength || !samples)
          return true;

        if (audioDataOffset + sampleLength > samples)
          return false;

        let dataView = new DataView(audioData.buffer);

        for (var i = 0; i < sampleLength; i++) {
          var value = outbuf[i << 1] + (outbuf[(i << 1) + 1] << 8);
          if (value >= 0x8000) value |= ~0x7FFF;
          // XXX: To investigate, this does not work directly on a FLoat32Array,
          // need to use a seperate DataView.
          dataView.setFloat32((audioDataOffset++) << 2, value / 0x8000, true);
        }

        return true;
      }

      flushBuffer();

      while (true) {
        let outDataType = ctypes.int16_t(0);
        gotBytes = ctypes.int16_t(0);
        let rv = PicoWrapper.getData(PicoWrapper.Engine, outbuf, 256,
                                     gotBytes.address(), outDataType.address());
        PicoWrapper.throwExceptionOnError('getData', rv);

        if (!sampleRate) {
          if (outDataType.value == PicoWrapper.PICO_DATA_PCM_16BIT)
            sampleRate = 16000;
          else
            throw 'Unknown pico audio data type';

          samples = sampleRate * (aMilliseconds / 1000);
          audioData = new Float32Array(samples);

          self.postMessage({type: 'audioInfo',
                            sampleRate: sampleRate,
                            channels: 1});
        }

        if (rv != 201) {
          if (!feedEngine())
            return audioData.subarray(0, audioDataOffset);
        }

        if (!flushBuffer())
          return audioData.subarray(0, audioDataOffset);
      }

      return new Float32Array(0);
    }

    feedEngine();
    let data = getAudio(400, true);
    this._getDataFunc = function() {
      if (data && data.length) {
        let now = Date.now();
        // XXX: TO investigate, in debug builds passing this data takes 300 ms.
        self.postMessage({type: 'audioData', audio: data});
        this.getAudioIntervalID = setInterval(
          function() {
            data = getAudio(100);
            self.postMessage({type: 'audioData', audio: data});
            if (!data || !data.length)
              clearInterval(this.getAudioIntervalID);
          }.bind(this), 100);
      }
    }.bind(this);
  },

  getData: function getData() {
    let getDataFunc = this._getDataFunc;
    if (getDataFunc) {
      this._getDataFunc = null;
      getDataFunc();
    }
  },

  resetEngine: function resetEngine() {
    clearInterval(this.getAudioIntervalID);
    this.getAudioIntervalID = 0;

    let rv = PicoWrapper.resetEngine(PicoWrapper.Engine,
                                     PicoWrapper.PICO_RESET_SOFT);
    PicoWrapper.throwExceptionOnError('resetEngine', rv);
  }
};

self.onmessage = function(event) {
  try {
    switch (event.data.type) {
      case 'synthesize':
        PicoTTSWorker.synthesize(event.data.utterance, event.data.options);
        break;
      case 'getData':
        if (PicoTTSWorker.getData)
          PicoTTSWorker.getData();
        break;
      case 'init':
        PicoTTSWorker.init(event.data.dataDir);
        break;
    }
  } catch (x) {
    dump('Pico TTS: Worker error: ' + x + ' (' + x.fileName + ':' +
         x.lineNumber + ':' + x.columnNumber + ')\n');
  }
};
