var Cc = Components.classes;
var Ci = Components.interfaces;
var Cu = Components.utils;
var Cr = Components.results;

Cu.import('resource://gre/modules/Services.jsm');
Cu.import('resource://gre/modules/FileUtils.jsm');

var gEnv = Cc['@mozilla.org/process/environment;1'].
  getService(Ci.nsIEnvironment);

var EXPORTED_SYMBOLS = ['tts'];

function AudioEngine() {}

AudioEngine.prototype = {
  synthesize: function AudioEngine_synthesize(aUtterance) {},
  play: function AudioEngine_play(aPlayer, aCallback) {}
};

function EarconEngine(aWindow) {
  this.window = aWindow;
  this.earcons = {};
  this.toplay = null;
}

EarconEngine.prototype = {
  addEarcon: function EarconEngine_addEarcon(aName, aURI, aCallback) {
    let audioElement = new this.window.Audio(aURI);
    audioElement.addEventListener(
      'loadedmetadata',
      function(e) {
        this.earcons[aName] = {element: audioElement,
                               channels: audioElement.mozChannels,
                               sampleRate: audioElement.mozSampleRate};
        if (aCallback)
          aCallback(true);
      }.bind(this));
    audioElement.preload = 'auto';
    audioElement.load();
  },

  synthesize: function EarconEngine_synthesize(aUtterance, aOptions, aCallback) {
    LOG('EarconEngine_synthesize', aUtterance);
    this.toplay = aUtterance;
    if (aCallback)
      aCallback();
  },

  play: function EarconEngine_play(aPlayer, aCallback) {
    LOG('EarconEngine_play');
    if (!this.toplay)
      return;

    let earcon = this.earcons[this.toplay];
    let audioElement = earcon.element;

    aPlayer.loadAndPlay(
      audioElement,
      function(aEvent) {
        this._stopFunc = null;
        if (aCallback)
          aCallback(aEvent);
      }.bind(this));

    this.toplay = null;
  }
};

function SpeechEngine(aWorkerURI, aDataDir) {
  this.workeruri = aWorkerURI;
  this.dataDir = aDataDir;
  this.audioInfo = null;
}

SpeechEngine.prototype = {
  __proto__: AudioEngine.prototype,

  init: function(aCallback) {
    this.worker = new ChromeWorker(this.workeruri);
    this.worker.onmessage = function(message) {
      if (message.data.type == 'voices') {
        this.voices = message.data.voices;
        let success = this.voices.length > 0;

        if (aCallback)
          aCallback(success);

        if (!success)
          this.uninit();
      }
    }.bind(this);

    this.worker.postMessage({type: 'init', dataDir: this.dataDir});
  },

  uninit: function() {
    this.worker.onmessage = function(message) {
      if (message.data.type == 'cleanedUp')
        self.worker.terminate();
    };
    this.worker.postMessage({type: 'uninit'});
  },

  synthesize: function SpeechEngine_synthesize(aUtterance, aOptions, aCallback) {
    LOG('SpeechEngine_synthesize', aUtterance);
    this.worker.onmessage = function(message) {
      if (message.data.type == 'audioInfo') {
        this.synthInfo = {channels: message.data.channels,
                          sampleRate: message.data.sampleRate,
                          utterance: aUtterance};
        if (aCallback)
          aCallback();
        }
    }.bind(this);

    this.worker.postMessage({type: 'synthesize',
                             utterance: aUtterance,
                             options: aOptions || {}});
  },

  play: function SpeechEngine_play(aPlayer, aCallback) {
    LOG('SpeechEngine_play', this.synthInfo);
    if (!this.synthInfo)
      throw 'Synthisis not complete.';

    let now = Date.now();

    let handle = aPlayer.open(
      this.synthInfo.channels, this.synthInfo.sampleRate,
      function(aEvent) {
        LOG('Setting playerInfo to null');
        this.playerInfo = null;
        if (aCallback)
          aCallback(aEvent);
      }.bind(this));

    this.worker.onmessage = function(message) {
      if (now) {
        LOG('latency:', Date.now() - now);
        now = 0;
      }
      if (message.data.audio.length)
        aPlayer.write(handle, message.data.audio);
      else
        aPlayer.close(handle);
    };

    this.synthInfo = null;
    this.worker.postMessage({type: 'getData'});
  }
};

function AudioPlayer(aAudioElement) {
  this.audioElement = aAudioElement;
  this.window = aAudioElement.ownerDocument.defaultView;
  this.doneTimeout = 0;
  this.totalSamples = 0;
  this.sampleRate = 0;
}

AudioPlayer.prototype = {
  open: function AudioPlayer_open(aChannels, aSampleRate, aCallback) {
    this.audioElement.src = null;
    this.audioElement.mozSetup(aChannels, aSampleRate);
    this.totalSamples = 0;
    this.sampleRate = aSampleRate;
    this.channels = aChannels;
    this.currentHandle = Date.now();

    let oldCallback = this.callback;
    this.callback = aCallback;
    if (oldCallback)
      oldCallback({type: 'interrupted', from: 'open'});

    return this.currentHandle;
  },

  write: function AudioPlayer_write(aHandle, aData) {
    if (!this.sampleRate || aHandle != this.currentHandle)
      return;

    this.audioElement.mozWriteAudio(aData);
    this.totalSamples += aData.length;
  },

  close: function AudioPlayer_close(aHandle) {
    if (aHandle != this.currentHandle)
      return;

    let samplesToPlay =
      this.totalSamples - this.audioElement.mozCurrentSampleOffset();
    this.doneTimeout = this.window.setTimeout(
      function done() {
        this.totalSamples = 0;
        let callback = this.callback;
        this.callback = null;
        if (callback)
          callback({type: 'end', from: 'done'});
      }.bind(this), Math.round(samplesToPlay / this.sampleRate * 1000));
  },

  stop: function AudioPlayer_stop() {
    if (this.currentHandle > 0)
      this.open(this.channels, this.sampleRate);
    else
      this.audioElement.pause();
  },

  loadAndPlay: function AudioPlayer_loadAndPlay(aAudioElement, aCallback) {
    let currentHandle = this.currentHandle = -Date.now();
    let audioElement = this.audioElement;
    let timeouthandler = 0;
    let window = audioElement.ownerDocument.defaultView;

    let oldCallback = this.callback;
    this.callback = null;
    if (oldCallback)
      oldCallback({type: 'interrupted', from: 'loadAndPlay'});

    function playEnd() {
      audioElement.removeEventListener('pause', playEnd);
      window.clearTimeout(timeouthandler);
      audioElement.src = null;
      if (aCallback)
        aCallback({type: 'end', from: 'playEnd'});
    }

    audioElement.mozLoadFrom(aAudioElement);
    timeouthandler = window.setTimeout(
      playEnd, Math.round(audioElement.duration * 1000));
    audioElement.addEventListener('pause', playEnd);
    audioElement.autoplay = true;
  }
};

var tts = {
  audioElement: null,
  builtinEngines: [
    {name: 'PicoTTS',
     worker: 'resource://gre/modules/PicoTTSWorker.js',
     dataPath: gEnv.get('PICO_LANG_PATH') || '/system/tts/lang_pico'}],
  engines: {},
  queue: [],
  current: null,

  init: function tts_init(aAudioElement, aCallback) {
    this.audioPlayer = new AudioPlayer(aAudioElement);
    this.earconEngine = new EarconEngine(
      aAudioElement.ownerDocument.defaultView);

    let engines = this.engines;
    let enginesToInitialize = this.builtinEngines.length;

    for each(let builtinEngine in this.builtinEngines) {
      let engine = new SpeechEngine(builtinEngine.worker,
                                    builtinEngine.dataPath);
      engine.init(
        function(success) {
          if (success) {
            engines[builtinEngine.name] = engine;
          }

          if (--enginesToInitialize == 0 && aCallback)
            aCallback();
        });
    }
  },

  addEarcon: function addEarcon(aName, aURI, aCallback) {
    this.earconEngine.addEarcon(aName, aURI, aCallback);
  },

  _play: function _play(aUtterance, aOptions, aCallback, aEarcon) {
    let engine = (aEarcon) ? this.earconEngine : this._chooseEngine(aOptions);
    if (!engine)
      return;

    let enqueue = !!(aOptions && aOptions.enqueue);
    let playTask = {
      engine: engine,
      utterance: aUtterance,
      options: aOptions,
      callback: function callbackWrapper(aEvent) {
        if (aOptions && aOptions.onEvent)
          aOptions.onEvent(aEvent);

        if (aEvent.type == 'end' || aEvent.type == 'interrupted')
          this._advanceQueue();
      }.bind(this)
    };

    let currentEngine = (this.current) ? this.current.engine : null;
    if (currentEngine != engine) {
      playTask.synthesizing = 'yes';
      engine.synthesize(
        aUtterance,
        aOptions,
        function() {
          playTask.synthesizing = 'done';
          if (playTask.synthCallback)
            playTask.synthCallback();
        });
    }

    if (enqueue) {
      this.queue.push(playTask);
    } else {
      this.queue = [playTask];
    }

    if (!this.current)
      this._advanceQueue();
    else if (!enqueue)
      this.audioPlayer.stop();
  },

  speak: function speak(aUtterance, aOptions, aCallback) {
    this._play(aUtterance, aOptions, aCallback);
  },

  playEarcon: function playEarcon(aUtterance, aOptions, aCallback) {
    this._play(aUtterance, aOptions, aCallback, true);
  },

  _advanceQueue: function _advanceQueue() {
    LOG('advance queue');
    let playTask = this.queue.shift();
    this.current = playTask;

    if (!playTask)
      return;

    let playFunc = function playFunc() {
      playTask.engine.play(this.audioPlayer, playTask.callback);
    }.bind(this);

    switch (playTask.synthesizing) {
      case 'done':
        playFunc();
        break;
      case 'yes':
        playTask.synthCallback = playFunc;
        break;
      default:
        playTask.engine.synthesize(playTask.utterance,
                                   playTask.options,
                                   playFunc);
        break;
    }
  },

  _chooseEngine: function _chooseEngine(aOptions) {
    for each(let engine in this.engines) {
      return engine;
    }
  },

  getVoices: function getVoices(aCallback) {
    let voices = [];
    for each(let engine in this.engines) {
      Array.prototype.push.apply(voices, engine.voices);
    }

    if (aCallback)
      aCallback(voices);
  }
};

function LOG() {
  let args = Array.prototype.slice.call(arguments);
  dump('TTS: ' + args.join(' ') + '\n');
}
