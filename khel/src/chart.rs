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

impl std::ops::Add<f64> for Beat {
  type Output = Self;
  fn add(self, other: f64) -> Beat {
    Beat(self.0 + other)
  }
}

impl std::ops::AddAssign<f64> for Beat {
  fn add_assign(&mut self, other: f64) {
    *self = Beat(self.0 + other);
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

// #[derive(Clone, Debug)]
// pub struct HoldInformation {
//   pub length: f64,
//   pub tick_count: u32,
// }

#[derive(Clone, Debug)]
pub enum HitObjectType {
  Hit,
  Hold,
  HoldTick,
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
  /// Return the color of this HitObject.
  pub fn color(&self) -> &str {
    let rows: u8 = self.keys
      .iter()
      .map(|&c| c.row().unwrap())
      .sum();
    match rows {
      1 => "red",
      2 => "green",
      3 => "yellow",
      4 => "blue",
      5 => "magenta",
      6 => "cyan",
      7 => "white",
      _ => unreachable!(),
    }
  }
  /// Return the asset that should be used to render this HitObject.
  pub fn asset(&self) -> String {
    let t = match self.t {
      HitObjectType::Hit | HitObjectType::Hold => "circle",
      HitObjectType::HoldTick => "hold_tick",
    };
    let color = self.color();
    format!("{}_{}", t, color)
  }
}

#[derive(Clone, Debug)]
/// A timing line.
pub struct TimingLine {
  pub beat: Beat,
}

impl TimingLine {
  /// Create a new TimingLine at the given beat.
  /// The beat can be fractional.
  pub fn at_beat(beat: Beat) -> Self {
    TimingLine {
      beat,
    }
  }
  /// Return the asset that should be used to render this TimingLine.
  pub fn asset(&self) -> String {
    let e = 1.0 / 2147483648.0;
    let f = self.beat.0.fract();
    let color = match f {
      0.0 => "red", // 1/4
      0.5 => "blue", // 1/8
      0.25 | 0.75 => "yellow", // 1/16
      0.125 | 0.375 | 0.625 | 0.875 => "green", // 1/32
      _ => {
        if (f - 0.33333f64).abs() < e || (f - 0.66666f64).abs() < e {
          "magenta"
        } else if (f - 0.16666f64).abs() < e || (f - 0.83333f64).abs() < e {
          "cyan"
        } else {
          "white"
        }
      },
    };
    format!("line_{}", color)
  }
}

#[derive(Clone, Debug)]
/// Some kind of struct that's synced to a beat.
pub enum SyncedStruct {
  HitObject(HitObject),
  TimingLine(TimingLine),
}

#[derive(Clone, Debug, Default)]
/// Wrapper over Vec<SyncedStruct>.
pub struct SyncedStructList(pub Vec<SyncedStruct>);

impl SyncedStructList {
  /// Create a SyncedStructList from a String.
  ///
  /// # Examples
  ///
  /// ```
  /// use khel::chart::SyncedStructList;
  /// // single hit
  /// let synced_struct_list = SyncedStructList::from_string(String::from("a@0"));
  /// // single hit using multiple keys
  /// let synced_struct_list = SyncedStructList::from_string(String::from("qaz@0"));
  /// // multiple hits on the same beat
  /// let synced_struct_list = SyncedStructList::from_string(String::from("a-b-c@0"));
  /// // one hit and one hold on the same beat
  /// let synced_struct_list = SyncedStructList::from_string(String::from("a+b:1=4@0"));
  /// // multiple hits across multiple beats
  /// let synced_struct_list = SyncedStructList::from_string(String::from("a@0,b@4,c@8"));
  /// ```
  pub fn from_string(s: String) -> Result<Self, anyhow::Error> {
    // example string: a-s+d-f:4=8@0,g-h+j-k:4=8@4
    let mut v: Vec<SyncedStruct> = vec![];
    // split on comma, yielding all the hit objects in the chart grouped by beat
    // e.g. ["a-s+d-f:4=8@0", "g-h-j-k:4=8@4"]
    let groupings: Vec<&str> = s.split(',').collect();
    for group in groupings.iter() {
      // split each group on at sign, yielding all the hit objects in the group and the beat they
      // start on
      // e.g. ["a-s+d-f:4=8", "0"]
      let hit_objects_and_beat: Vec<&str> = group.split('@').collect();
      if hit_objects_and_beat.len() > 2 {
        return Err(SyncedStructListError::MultipleAtSigns.into());
      }
      let hit_objects = hit_objects_and_beat.first().unwrap().to_string();
      let Some(beat) = hit_objects_and_beat.get(1) else {
        return Err(SyncedStructListError::MissingBeatValue.into());
      };
      let beat = beat.parse::<f64>()?;
      let beat = Beat::from_f64(beat);
      // create a single timing line synced to the whole grouping
      let timing_line = TimingLine::at_beat(beat);
      v.push(SyncedStruct::TimingLine(timing_line));
      // split the hit objects on plus sign, separating the hits from the holds
      // e.g. ["a-s", "d-f:4=8"]
      let hits_and_holds: Vec<&str> = hit_objects.split('+').collect();
      if hits_and_holds.len() > 2 {
        return Err(SyncedStructListError::MultiplePlusSigns.into());
      }
      // TODO: how many times do the hit and hold loops iterate?
      // split the hits on dash, then cast them, yielding a Vec<String>
      // e.g. ["a", "s"]
      let hits: Vec<String> = hits_and_holds.first().unwrap_or(&"").split('-').map(String::from).collect();
      for hit in hits.iter() {
        if hit.is_empty() {
          continue;
        }
        if !hit.chars().all_unique() {
          return Err(SyncedStructListError::DuplicateHitChar.into());
        }
        if !hit.chars().map(|c| c.column()).all_equal() {
          return Err(SyncedStructListError::MultiColumnHit.into());
        }
        let hit_object = HitObject::at_beat(
          beat,
          HitObjectType::Hit,
          hit.chars().collect(),
        );
        v.push(SyncedStruct::HitObject(hit_object));
      }
      // split the holds on colon, yielding the holds and their required information
      // e.g. ["d-f", "4=8"]
      let Some(holds_and_info) = hits_and_holds.get(1) else { continue; };
      let holds_and_info: Vec<&str> = holds_and_info.split(':').collect();
      if holds_and_info.len() > 2 {
        return Err(SyncedStructListError::MultipleColons.into());
      }
      // split the holds on dash, then cast them, yielding a Vec<String>
      let holds: Vec<String> = holds_and_info.first().unwrap_or(&"").split('-').map(String::from).collect();
      let Some(info) = holds_and_info.get(1) else {
        return Err(SyncedStructListError::MissingHoldInformation.into());
      };
      // split the hold information on equals, separating the length from the tick count
      // e.g. ["4", "8"]
      let length_and_tick_count: Vec<&str> = info.split('=').collect();
      if length_and_tick_count.len() > 2 {
        return Err(SyncedStructListError::MultipleEqualsSigns.into());
      }
      let length = length_and_tick_count.first().unwrap();
      let length = length.parse::<f64>()?;
      let Some(tick_count) = length_and_tick_count.get(1) else {
        return Err(SyncedStructListError::MissingHoldTickCount.into());
      };
      let tick_count = tick_count.parse::<u32>()?;
      // holds
      for hold in holds.iter() {
        if hold.is_empty() {
          continue;
        }
        if !hold.chars().all_unique() {
          return Err(SyncedStructListError::DuplicateHoldChar.into());
        }
        if !hold.chars().map(|c| c.column()).all_equal() {
          return Err(SyncedStructListError::MultiColumnHold.into());
        }
        // let hold_information = HoldInformation {
        //   length,
        //   tick_count,
        // };
        let hit_object = HitObject::at_beat(
          beat,
          HitObjectType::Hold,
          hold.chars().collect(),
        );
        v.push(SyncedStruct::HitObject(hit_object));
      }
      // hold ticks
      for hold in holds.iter() {
        let mut i = 1;
        let delta = length / tick_count as f64;
        let mut tick_beat = beat + delta; // skip the tick that would land on the start of the hold
        while i < tick_count {
          let hit_object = HitObject::at_beat(
            tick_beat,
            HitObjectType::HoldTick,
            hold.chars().collect(),
          );
          v.push(SyncedStruct::HitObject(hit_object));
          i += 1;
          tick_beat += delta;
        }
      }
    }
    Ok(SyncedStructList(v))
  }
}

#[derive(Debug, Error)]
pub enum SyncedStructListError {
  #[error("missing beat value")]
  MissingBeatValue,
  #[error("missing hold information")]
  MissingHoldInformation,
  #[error("missing hold tick count")]
  MissingHoldTickCount,
  #[error("multiple at signs")]
  MultipleAtSigns,
  #[error("multiple plus signs")]
  MultiplePlusSigns,
  #[error("multiple equals signs")]
  MultipleEqualsSigns,
  #[error("multiple colons")]
  MultipleColons,
  #[error("found duplicate hit char")]
  DuplicateHitChar,
  #[error("attempted to create hit across multiple columns")]
  MultiColumnHit,
  #[error("found duplicate hold char")]
  DuplicateHoldChar,
  #[error("attempted to create hold across multiple columns")]
  MultiColumnHold,
}

#[derive(Debug)]
pub struct Chart {
  pub metadata: Metadata,
  pub audio: Sound,
  pub synced_structs: SyncedStructList,
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
    info!("creating synced structs list...");
    let synced_structs = SyncedStructList::from_string(hit_objects)?;
    info!("created {} synced structs", synced_structs.0.len());
    info!("finished!");
    // chart
    let chart = Chart {
      metadata,
      audio,
      synced_structs,
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
      synced_structs: SyncedStructList::default(),
    }
  }
  /// Mutate this Chart according to the specified ratemod.
  pub fn set_ratemod(&mut self, ratemod: f64) {
    for bpm in self.metadata.bpms.0.iter_mut() {
      bpm.value *= ratemod;
    }
    self.audio.set_speed(ratemod);
  }
  /// Begin playing this chart.
  pub fn play(&self) {
    let Metadata { title, subtitle, artist, credit, .. } = &self.metadata;
    match subtitle.as_str() {
      "" => info!("playing chart \"{} - {}\" (mapped by {})...", artist, title, credit),
      _ => info!("playing chart \"{} - {} ({})\" (mapped by {})...", artist, title, subtitle, credit),
    };
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
