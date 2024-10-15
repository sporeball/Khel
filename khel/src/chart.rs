use crate::{read_lines, sound::Sound, traits::KeyboardPosition};
use std::collections::HashMap;
use std::path::Path;
// use std::time::Duration;
use itertools::Itertools;
use log::info;
use thiserror::Error;
// use winit::dpi::PhysicalSize;

pub const CHART_VERSION: u8 = 0;

#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Beat(pub f64);

impl Beat {
  /// Create a Beat from an f64.
  pub fn from_f64(f: f64) -> Self {
    Beat(f)
  }
  /// Convert this Beat to an exact time in seconds.
  pub fn to_exact_time(&self, bpms: &BpmList) -> f64 {
    let bpms = &bpms.0;
    let mut beats_remaining = self.0;
    let mut exact_time = 0.0;
    for (i, bpm) in bpms.iter().enumerate() {
      let one_minute = 60.0;
      let one_beat = one_minute / bpm.value;
      let Some(next_bpm) = bpms.get(i + 1) else {
        exact_time += one_beat * beats_remaining;
        break;
      };
      let length = bpm.length(Some(next_bpm));
      if beats_remaining < length / one_beat {
        exact_time += one_beat * beats_remaining;
        break;
      } else {
        exact_time += length;
        beats_remaining -= length / one_beat;
      }
    }
    exact_time
  }
}

#[derive(Clone, Debug, Default, PartialEq)]
pub struct Bpm {
  pub value: f64,
  pub start_beat: Beat,
}

impl Bpm {
  /// Create a Bpm from a String.
  /// When possible, prefer creating a BpmList instead.
  ///
  /// # Examples
  ///
  /// ```
  /// use khel::chart::Bpm;
  /// // 120bpm starting at beat 0
  /// let bpm = Bpm::from_string(String::from("120@0"));
  /// ```
  pub fn from_string(s: String) -> Result<Self, anyhow::Error> {
    let v: Vec<&str> = s.split('@').collect();
    if v.len() > 2 {
      return Err(BpmError::TooManyParts.into());
    }
    let value = v.first().unwrap().parse::<f64>()?;
    let Some(start_beat) = v.get(1) else { return Err(BpmError::MissingStartBeat.into()); };
    let start_beat = start_beat.parse::<f64>()?;
    let start_beat = Beat::from_f64(start_beat);
    Ok(Bpm {
      value,
      start_beat,
    })
  }
  /// Return the length of this Bpm in seconds.
  pub fn length(&self, next_bpm: Option<&Bpm>) -> f64 {
    let Some(next_bpm) = next_bpm else { return f64::MAX; };
    let beats = next_bpm.start_beat.0 - self.start_beat.0;
    let one_minute = 60.0;
    let one_beat = one_minute / self.value;
    beats * one_beat
  }
}

#[derive(Debug, Error)]
pub enum BpmError {
  #[error("attempted to create bpm with too many parts")]
  TooManyParts,
  #[error("missing bpm start beat")]
  MissingStartBeat,
}

#[derive(Clone, Debug, Default)]
pub struct BpmList(pub Vec<Bpm>);

impl BpmList {
  /// Create a BpmList from a single f64.
  pub fn from_f64(value: f64) -> Self {
    let bpm = Bpm {
      value,
      start_beat: Beat::from_f64(0.0),
    };
    BpmList(vec![bpm])
  }
  /// Create a BpmList from a string.
  ///
  /// # Examples
  ///
  /// ```
  /// use khel::chart::BpmList;
  /// // single bpm
  /// let bpm_list = BpmList::from_string(String::from("120@0"));
  /// // multiple bpms
  /// let bpm_list = BpmList::from_string(String::from("120@0,140@16"));
  /// ```
  pub fn from_string(s: String) -> Result<Self, anyhow::Error> {
    if s.is_empty() {
      return Err(BpmListError::EmptyString.into());
    }
    let mut v: Vec<Bpm> = vec![];
    let bpms: Vec<&str> = s.split(',').collect();
    for bpm in bpms {
      let bpm = Bpm::from_string(bpm.to_string())?;
      v.push(bpm);
    }
    Ok(BpmList(v))
  }
  /// Return a reference to the Bpm from this BpmList that should be used at a given exact time.
  pub fn at_exact_time(&self, exact_time: f64) -> &Bpm {
    let bpms = &self.0;
    if bpms.len() == 1 {
      return &bpms[0];
    }
    if exact_time <= 0.0 {
      return &bpms[0];
    }
    let mut i = 0;
    let mut time = exact_time;
    loop {
      let length = self.0[i].length(self.0.get(i + 1));
      if time >= length {
        time -= length;
        i += 1;
      } else {
        return &self.0[i];
      }
    }
  }
  /// Return a reference to the maximum Bpm from this BpmList.
  pub fn max(&self) -> &Bpm {
    self.0.iter().sorted_by(|a, b| a.value.partial_cmp(&b.value).unwrap()).last().unwrap()
  }
}

#[derive(Debug, Error)]
pub enum BpmListError {
  #[error("attempted to create bpm list from an empty string")]
  EmptyString,
}

#[derive(Debug, Default)]
pub struct Metadata {
  pub version: u8,
  pub title: String,
  pub subtitle: String,
  pub artist: String,
  pub credit: String,
  pub bpms: BpmList,
}

impl Metadata {
  pub fn new(
    version: u8,
    title: String,
    subtitle: String,
    artist: String,
    credit: String,
    bpms: BpmList,
  ) -> Self {
    Metadata {
      version,
      title,
      subtitle,
      artist,
      credit,
      bpms,
    }
  }
}

#[derive(Clone, Debug)]
pub enum HitObjectType {
  Hit,
  Hold,
}

#[derive(Clone, Debug)]
pub struct HitObject {
  pub t: HitObjectType,
  pub keys: Vec<char>,
  pub beat: Beat,
}

impl HitObject {
  /// Create a new HitObject at the given beat.
  /// The beat can be fractional.
  /// When possible, prefer creating a HitObjectList instead.
  pub fn at_beat(beat: Beat, t: HitObjectType, keys: Vec<char>) -> Self {
    HitObject {
      t,
      keys,
      beat,
    }
  }
  /// Return the lane that this HitObject is in.
  pub fn lane(&self) -> u8 {
    self.keys[0].column().unwrap()
  }
  /// Return the x-coordinate of the lane that this HitObject is in.
  pub fn lane_x(&self) -> f32 {
    (0.1f32 * self.lane() as f32) - 0.45
  }
  /// Return the asset that should be used to render this HitObject.
  pub fn asset(&self) -> &str {
    let rows: u8 = self.keys
      .iter()
      .map(|&c| c.row().expect("found invalid key in hit object"))
      .sum();
    match rows {
      1 => "circle_red",
      2 => "circle_green",
      3 => "circle_yellow",
      4 => "circle_blue",
      5 => "circle_magenta",
      6 => "circle_cyan",
      7 => "circle_white",
      _ => unreachable!(),
    }
  }
  /// Return the asset that should be used to render a hold tick of the same color as this
  /// HitObject.
  // TODO: better split
  pub fn hold_tick_asset(&self) -> &str {
    let rows: u8 = self.keys
      .iter()
      .map(|&c| c.row().expect("found invalid key in hit object"))
      .sum();
    match rows {
      1 => "hold_tick_red",
      2 => "hold_tick_green",
      3 => "hold_tick_yellow",
      4 => "hold_tick_blue",
      5 => "hold_tick_magenta",
      6 => "hold_tick_cyan",
      7 => "hold_tick_white",
      _ => unreachable!(),
    }
  }
}

#[derive(Clone, Debug, Default)]
/// Wrapper over Vec<HitObject>.
pub struct HitObjectList(pub Vec<HitObject>);

impl HitObjectList {
  /// Create a HitObjectList from a String.
  ///
  /// # Examples
  ///
  /// ```
  /// use khel::chart::HitObjectList;
  /// // single hit
  /// let hit_object_list = HitObjectList::from_string(String::from("a@0"));
  /// // single hit using multiple keys
  /// let hit_object_list = HitObjectList::from_string(String::from("qaz@0"));
  /// // multiple hits on the same beat
  /// let hit_object_list = HitObjectList::from_string(String::from("a-b-c@0"));
  /// // one hit and one hold on the same beat
  /// let hit_object_list = HitObjectList::from_string(String::from("a+b@0"));
  /// // multiple hits across multiple beats
  /// let hit_object_list = HitObjectList::from_string(String::from("a@0,b@4,c@8"));
  /// ```
  pub fn from_string(s: String) -> Result<Self, anyhow::Error> {
    let mut v: Vec<HitObject> = vec![];
    // Vec of what would have been `Tick`s in the old system.
    // each element consists of one or more hit objects which should have the same exact_time.
    // e.g. a-f-k@0
    let synced: Vec<&str> = s.split(',').collect();
    for group in synced.iter() {
      let g: Vec<&str> = group.split('@').collect();
      if g.len() > 2 {
        panic!("too many parts");
      }
      let hit_objects = g.first().unwrap().to_string();
      let Some(beat) = g.get(1) else {
        panic!("missing beat value");
      };
      let beat = beat.parse::<f64>()?;
      let beat = Beat::from_f64(beat);
      // create hit objects
      let h: Vec<&str> = hit_objects.split('+').collect();
      // e.g. ["a", "f", "k"]
      let hits: Vec<String> = h.first().unwrap_or(&"").split('-').map(String::from).collect();
      let holds: Vec<String> = h.get(1).unwrap_or(&"").split('-').map(String::from).collect();
      // hits
      for hit in hits.iter() {
        if hit.is_empty() {
          continue;
        }
        if !hit.chars().all_unique() {
          return Err(HitObjectListError::DuplicateHitChar.into());
        }
        if !hit.chars().map(|c| c.column()).all_equal() {
          return Err(HitObjectListError::MultiColumnHit.into());
        }
        let hit_object = HitObject::at_beat(
          beat,
          HitObjectType::Hit,
          hit.chars().collect(),
        );
        v.push(hit_object);
      }
      // holds
      for hold in holds.iter() {
        if hold.is_empty() {
          continue;
        }
        if !hold.chars().all_unique() {
          return Err(HitObjectListError::DuplicateHoldChar.into());
        }
        if !hold.chars().map(|c| c.column()).all_equal() {
          return Err(HitObjectListError::MultiColumnHold.into());
        }
        let hit_object = HitObject::at_beat(
          beat,
          HitObjectType::Hold,
          hold.chars().collect(),
        );
        v.push(hit_object);
      }
    }
    Ok(HitObjectList(v))
  }
}

#[derive(Debug, Error)]
pub enum HitObjectListError {
  #[error("found duplicate hit char")]
  DuplicateHitChar,
  #[error("attempted to create hit across multiple columns")]
  MultiColumnHit,
  #[error("found duplicate hold char")]
  DuplicateHoldChar,
  #[error("attempted to create hold across multiple columns")]
  MultiColumnHold,
}

//  /// Return the asset that should be used to render this tick's timing line.
//  pub fn timing_line_asset(&self, divisor: &Divisor) -> Result<&str, TickError> {
//    let asset = match divisor.value {
//      1 | 2 | 4 => "line_red",
//      6 => match divisor.units_elapsed % 6 {
//        0 | 3 => "line_red",
//        _ => "line_magenta",
//      },
//      8 => match divisor.units_elapsed % 8 {
//        0 | 4 => "line_red",
//        _ => "line_blue",
//      },
//      12 => match divisor.units_elapsed % 12 {
//        0 | 3 | 6 | 9 => "line_red",
//        _ => "line_magenta",
//      }
//      16 => match divisor.units_elapsed % 16 {
//        0 | 4 | 8 | 12 => "line_red",
//        2 | 6 | 10 | 14 => "line_blue",
//        _ => "line_yellow",
//      },
//      24 => match divisor.units_elapsed % 24 {
//        0 | 6 | 12 | 18 => "line_red",
//        3 | 9 | 15 | 21 => "line_magenta",
//        _ => "line_cyan",
//      },
//      32 => match divisor.units_elapsed % 32 {
//        0 | 8 | 16 | 24 => "line_red",
//        4 | 12 | 20 | 28 => "line_blue",
//        2 | 6 | 10 | 14 | 18 | 22 | 26 | 30 => "line_yellow",
//        _ => "line_green",
//      },
//      _ => return Err(TickError::UnsupportedDivisor),
//    };
//    Ok(asset)
//  }
//}

#[derive(Debug)]
pub struct Chart {
  pub metadata: Metadata,
  pub audio: Sound,
  // pub ticks: TickList,
  pub hit_objects: HitObjectList,
}

impl Chart {
  /// Write this chart to disk as a .khel file.
  pub fn write_to_disk(&self, _filename: String) -> Result<(), anyhow::Error> {
    // let mut file = File::create(filename)?;
    // TODO
    Ok(())
  }
  /// Read a chart from a .khel file into a Chart object.
  pub fn read_from_disk<P>(filename: P) -> Result<Chart, anyhow::Error>
  where
    P: AsRef<Path> + std::fmt::Debug,
  {
    info!("reading {:?} from disk...", filename);
    let mut map: HashMap<String, String> = HashMap::new();
    let lines = read_lines(filename)?;
    for line in lines.map_while(Result::ok) { // maps Result<String> to String
      if line.is_empty() {
        continue;
      }
      let (key, value) = deserialize_kv(line)?;
      map.insert(key, value);
    }
    info!("found {} key-value pairs", map.keys().len());
    // required key-value pairs
    let Some(version) = map.get("version") else { return Err(ChartError::MissingKeyValuePair(String::from("version")).into()); };
    let version = version.parse::<u8>()?;
    let Some(title) = map.get("title") else { return Err(ChartError::MissingKeyValuePair(String::from("title")).into()); };
    let title = title.to_string();
    let Some(subtitle) = map.get("subtitle") else { return Err(ChartError::MissingKeyValuePair(String::from("subtitle")).into()); };
    let subtitle = subtitle.to_string();
    let Some(artist) = map.get("artist") else { return Err(ChartError::MissingKeyValuePair(String::from("artist")).into()); };
    let artist = artist.to_string();
    let Some(credit) = map.get("credit") else { return Err(ChartError::MissingKeyValuePair(String::from("credit")).into()); };
    let credit = credit.to_string();
    let Some(hit_objects) = map.get("hit_objects") else { return Err(ChartError::MissingKeyValuePair(String::from("hit_objects")).into()); };
    let hit_objects = hit_objects.to_string();
    // bpms
    let bpms = match (map.get("bpm"), map.get("bpms")) {
      (Some(bpm), None) => BpmList::from_f64(bpm.parse::<f64>()?),
      (None, Some(bpms)) => BpmList::from_string(bpms.to_string())?,
      (Some(_), Some(_)) => { return Err(ChartError::ConflictingBpmInformation.into()); },
      (None, None) => { return Err(ChartError::MissingBpmInformation.into()); },
    };
    // metadata
    info!("creating metadata...");
    let metadata = Metadata::new(
      version,
      title.clone(),
      subtitle.clone(),
      artist.clone(),
      credit,
      bpms.clone(),
    );
    // audio
    info!("creating audio object...");
    let audio_filename = match subtitle.as_str() {
      "" => format!("{} - {}.wav", artist, title),
      _ => format!("{} - {} ({}).wav", artist, title, subtitle),
    };
    let audio = Sound::new(&audio_filename);
    // ticks
    info!("creating hit object list...");
    let hit_objects = HitObjectList::from_string(hit_objects)?;
    info!("parsed {} ticks", hit_objects.0.len());
    info!("finished!");
    // chart
    let chart = Chart {
      metadata,
      audio,
      hit_objects,
    };
    info!("{:?}", chart);
    Ok(chart)
  }
  /// An empty chart.
  /// This is used internally when there is no chart being played.
  pub fn empty() -> Self {
    Chart {
      metadata: Metadata::default(),
      audio: Sound::empty(),
      hit_objects: HitObjectList::default(),
    }
  }
  /// Mutate this Chart according to the specified ratemod.
  pub fn set_ratemod(&mut self, ratemod: f64) {
    for bpm in self.metadata.bpms.0.iter_mut() {
      bpm.value *= ratemod;
    }
  }
  /// Begin playing this chart.
  pub fn play(&self, ratemod: f64) {
    let Metadata { title, subtitle, artist, credit, .. } = &self.metadata;
    match subtitle.as_str() {
      "" => info!("playing chart \"{} - {}\" (mapped by {})...", artist, title, credit),
      _ => info!("playing chart \"{} - {} ({})\" (mapped by {})...", artist, title, subtitle, credit),
    };
    self.audio.set_speed(ratemod);
  }
}

#[derive(Debug, Error)]
pub enum ChartError {
  #[error("missing key-value pair: {0}")]
  MissingKeyValuePair(String),
  #[error("found conflicting bpm information")]
  ConflictingBpmInformation,
  #[error("missing bpm information")]
  MissingBpmInformation,
}

/// Status of the currently active chart.
// #[derive(Default)]
#[derive(Debug)]
pub enum ChartStatus {
  // Countdown,
  // #[default]
  None,
  Paused,
  Playing,
}

#[derive(Debug)]
/// Info for KhelState about the currently active chart.
pub struct ChartInfo {
  pub chart: Chart,
  pub status: ChartStatus,
  pub start_time: f64,
}

impl ChartInfo {
  pub fn new(chart: Chart) -> Self {
    ChartInfo {
      chart,
      status: ChartStatus::None,
      start_time: 0.0,
    }
  }
  pub fn none() -> Self {
    ChartInfo {
      chart: Chart::empty(),
      status: ChartStatus::None,
      start_time: 0.0,
    }
  }
}

/// Serialize a key-value pair from a (String, String) into .khel format.
// fn serialize_kv(_raw: (String, String)) -> Result<String, anyhow::Error> {
//   // TODO
//   Ok(String::new())
// }

/// Deserialize a key-value pair from .khel format into a (String, String).
fn deserialize_kv(raw: String) -> Result<(String, String), DeserializationError> {
  let equals = raw.find('=');
  let Some(equals) = equals else { return Err(DeserializationError::MalformedKeyValuePair); };
  let key = &raw[0..equals];
  if !raw.ends_with(';') {
    return Err(DeserializationError::MalformedKeyValuePairWithValidKey(key.to_string()));
  }
  let value = &raw[equals+1..raw.len()-1];
  Ok((key.to_string(), value.to_string()))
}

#[derive(Debug, Error)]
pub enum DeserializationError {
  #[error("malformed key-value pair")]
  MalformedKeyValuePair,
  #[error("malformed key-value pair: {0}")]
  MalformedKeyValuePairWithValidKey(String),
}
