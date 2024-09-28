use crate::{read_lines, sound::Sound, traits::KeyboardPosition};
use std::collections::HashMap;
use std::path::Path;
use std::time::Duration;
use itertools::Itertools;
use log::info;
use thiserror::Error;

pub const CHART_VERSION: u8 = 0;

#[derive(Clone, Debug, Default)]
pub struct Bpm {
  pub value: f64,
  pub start_tick: u32,
}

impl Bpm {
  /// Create a Bpm from a String.
  /// When possible, prefer creating a BpmList instead.
  ///
  /// # Examples
  ///
  /// ```
  /// // 120bpm starting at tick 0
  /// let bpm = Bpm::from_string(String::from("120@0"));
  /// ```
  pub fn from_string(s: String) -> Result<Self, anyhow::Error> {
    let v: Vec<&str> = s.split('@').collect();
    if v.len() > 2 {
      return Err(BpmError::TooManyParts.into());
    }
    let value = v.first().unwrap().parse::<f64>()?;
    let Some(start_tick) = v.get(1) else { return Err(BpmError::MissingStartTick.into()); };
    let start_tick = start_tick.parse::<u32>()?;
    Ok(Bpm {
      value,
      start_tick,
    })
  }
}

#[derive(Debug, Error)]
pub enum BpmError {
  #[error("attempted to create bpm with too many parts")]
  TooManyParts,
  #[error("missing bpm start tick")]
  MissingStartTick,
}

#[derive(Clone, Debug, Default)]
pub struct BpmList(pub Vec<Bpm>);

impl BpmList {
  /// Create a BpmList from a single f64.
  pub fn from_f64(value: f64) -> Self {
    let bpm = Bpm {
      value,
      start_tick: 0,
    };
    BpmList(vec![bpm])
  }
  /// Create a BpmList from a string.
  ///
  /// # Examples
  ///
  /// ```
  /// // single bpm
  /// let bpm_list = BpmList::from_string(String::from("120@0"));
  /// // multiple bpms
  /// let bpm_list = BpmList::from_string(String:from("120@0,140@16"));
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
  /// Return a reference to the Bpm from this BpmList that should be used at a given tick.
  pub fn at_tick(&self, tick: u32) -> &Bpm {
    self.0.iter().filter(|d| d.start_tick <= tick).last().unwrap()
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

#[derive(Clone, Debug, Default)]
pub struct Divisor {
  pub value: u8,
  pub start_tick: u32,
  pub units_elapsed: u32,
}

impl Divisor {
  /// Create a Divisor from a String.
  /// When possible, prefer creating a DivisorList instead.
  ///
  /// # Examples
  ///
  /// ```
  /// // 1/16th divisor starting at tick 0
  /// let divisor = Divisor::from_string(String::from("16@0"));
  /// ```
  pub fn from_string(s: String) -> Result<Self, anyhow::Error> {
    let v: Vec<&str> = s.split('@').collect();
    if v.len() > 2 {
      return Err(DivisorError::TooManyParts.into());
    }
    let value = v.first().unwrap().parse::<u8>()?;
    let Some(start_tick) = v.get(1) else { return Err(DivisorError::MissingStartTick.into()); };
    let start_tick = start_tick.parse::<u32>()?;
    Ok(Divisor {
      value,
      start_tick,
      units_elapsed: 0,
    })
  }
}

#[derive(Debug, Error)]
pub enum DivisorError {
  #[error("attempted to create divisor with too many parts")]
  TooManyParts,
  #[error("missing divisor start tick")]
  MissingStartTick,
}

#[derive(Clone, Debug, Default)]
pub struct DivisorList(pub Vec<Divisor>);

impl DivisorList {
  /// Create a DivisorList from a single u8.
  pub fn from_u8(value: u8) -> Self {
    let divisor = Divisor {
      value,
      start_tick: 0,
      units_elapsed: 0,
    };
    DivisorList(vec![divisor])
  }
  /// Create a DivisorList from a string.
  ///
  /// # Examples
  ///
  /// ```
  /// // single divisor
  /// let divisor_list = DivisorList::from_string(String::from("16@0"));
  /// // multiple divisors
  /// let divisor_list = DivisorList::from_string(String::from("16@0,24@16"));
  /// ```
  pub fn from_string(s: String) -> Result<Self, anyhow::Error> {
    if s.is_empty() {
      return Err(DivisorListError::EmptyString.into());
    }
    let mut v: Vec<Divisor> = vec![];
    let divisors: Vec<&str> = s.split(',').collect();
    for divisor in divisors {
      let divisor = Divisor::from_string(divisor.to_string())?;
      v.push(divisor);
    }
    Ok(DivisorList(v))
  }
  /// Return a reference to the Divisor from this DivisorList that should be used at a given tick.
  pub fn at_tick(&self, tick: u32) -> &Divisor {
    self.0.iter().filter(|d| d.start_tick <= tick).last().unwrap()
  }
  /// Return a mutable reference to the Divisor from this DivisorList that should be used at a
  /// given tick.
  pub fn at_tick_mut(&mut self, tick: u32) -> &mut Divisor {
    self.0.iter_mut().filter(|d| d.start_tick <= tick).last().unwrap()
  }
}

#[derive(Debug, Error)]
pub enum DivisorListError {
  #[error("attempted to create divisor list from an empty string")]
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
  pub divisors: DivisorList,
}

impl Metadata {
  pub fn new(
    version: u8,
    title: String,
    subtitle: String,
    artist: String,
    credit: String,
    bpms: BpmList,
    divisors: DivisorList,
  ) -> Self {
    Metadata {
      version,
      title,
      subtitle,
      artist,
      credit,
      bpms,
      divisors,
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
}

impl HitObject {
  /// Create a new HitObject from a Vec<char> and HitObjectType.
  /// When possible, prefer creating a HitObjectList instead.
  pub fn from_keys(keys: Vec<char>, t: HitObjectType) -> Self {
    HitObject {
      t,
      keys,
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
      _ => unreachable!(), // i hope
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
  /// // one hit
  /// let hit_object_list = HitObjectList::from_string(String::from("az"));
  /// // three hits
  /// let hit_object_list = HitObjectList::from_string(String::from("a+b+c"));
  /// // one hit and one hold
  /// let hit_object_list = HitObjectList::from_string(String::from("a/b"));
  /// ```
  pub fn from_string(s: String) -> Result<Self, anyhow::Error> {
    // TODO: need to check how this behaves if you want to include a slash hit.
    let s: Vec<&str> = s.split('/').collect();
    let mut v: Vec<HitObject> = vec![];
    let hits: Vec<String> = s.first().unwrap_or(&"").split('+').map(String::from).collect();
    let holds: Vec<String> = s.get(1).unwrap_or(&"").split('+').map(String::from).collect();
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
      v.push(HitObject::from_keys(
        hit.chars().collect(),
        HitObjectType::Hit
      ));
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
      v.push(HitObject::from_keys(
        hold.chars().collect(),
        HitObjectType::Hold
      ));
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

// #[derive(Debug, Default)]
#[derive(Clone, Debug)]
pub struct Tick {
  pub length: u8,
  pub hit_objects: HitObjectList,
}

impl Tick {
  /// Create a Tick from a String.
  ///
  /// # Examples
  ///
  /// ```
  /// // a tick with one HitObject lasting 1 unit at 120bpm
  /// let tick = Tick::from_string(String::from("a:0@120"));
  /// ```
  pub fn from_string(s: String) -> Result<Self, anyhow::Error> {
    if s.is_empty() {
      return Err(TickError::EmptyString.into());
    }
    let v: Vec<&str> = s.split(':').collect();
    if v.len() > 2 {
      return Err(TickError::TooManyParts.into());
    }
    let head = v.first().unwrap();
    let Some(length) = v.get(1) else { return Err(TickError::MissingTickLength.into()); };
    let length = length.parse::<u8>()?;
    let hit_objects = HitObjectList::from_string(head.to_string())?;
    let tick = Tick {
      length,
      hit_objects,
    };
    Ok(tick)
  }
  /// Return the length of this Tick as a Duration.
  pub fn duration(&self, bpm: f64, divisor: u8, ratemod: f32) -> Duration {
    let divisor = divisor as f64;
    let ratemod = ratemod as f64;
    // 1-256
    let length = (self.length + 1) as f64;
    let one_bar = Duration::from_secs_f64((60f64 / (bpm * ratemod)) * 4.0);
    one_bar.div_f64(divisor).mul_f64(length)
  }
  /// Return the length of this tick in quarter notes.
  pub fn quarter_notes(&self, divisor: u8) -> f32 {
    let divisor = divisor as f32;
    // 1-256
    let length = (self.length + 1) as f32;
    let bars = length / divisor;
    bars * 4.0
  }
  /// Return the distance between zero and two from this tick to the next.
  pub fn distance(&self, ho_height: f32, divisor: u8, xmod: f32) -> f32 {
    // 1/4 = one height
    self.quarter_notes(divisor) * ho_height * xmod
  }
  /// Return the asset that should be used to render this tick's timing line.
  pub fn timing_line_asset(&self, divisor: &Divisor) -> Result<&str, TickError> {
    let asset = match divisor.value {
      1 | 2 | 4 => "line_red",
      6 => match divisor.units_elapsed % 6 {
        0 | 3 => "line_red",
        _ => "line_magenta",
      },
      8 => match divisor.units_elapsed % 8 {
        0 | 4 => "line_red",
        _ => "line_blue",
      },
      12 => match divisor.units_elapsed % 12 {
        0 | 3 | 6 | 9 => "line_red",
        _ => "line_magenta",
      }
      16 => match divisor.units_elapsed % 16 {
        0 | 4 | 8 | 12 => "line_red",
        2 | 6 | 10 | 14 => "line_blue",
        _ => "line_yellow",
      },
      24 => match divisor.units_elapsed % 24 {
        0 | 6 | 12 | 18 => "line_red",
        3 | 9 | 15 | 21 => "line_magenta",
        _ => "line_cyan",
      },
      32 => match divisor.units_elapsed % 32 {
        0 | 8 | 16 | 24 => "line_red",
        4 | 12 | 20 | 28 => "line_blue",
        2 | 6 | 10 | 14 | 18 | 22 | 26 | 30 => "line_yellow",
        _ => "line_green",
      },
      _ => return Err(TickError::UnsupportedDivisor),
    };
    Ok(asset)
  }
}

#[derive(Debug, Error)]
pub enum TickError {
  #[error("attempted to create tick from an empty string")]
  EmptyString,
  #[error("attempted to create tick with too many parts")]
  TooManyParts,
  #[error("missing tick length")]
  MissingTickLength,
  #[error("unsupported divisor")]
  UnsupportedDivisor,
}

#[derive(Debug)]
/// Info for KhelState about the current tick.
pub struct TimingInfo {
  pub instance_time: Duration,
  pub hit_time: Duration,
  pub end_time: Duration,
}

pub struct TimingInfoList(pub Vec<TimingInfo>);

impl TimingInfoList {
  pub fn new(v: Vec<TimingInfo>) -> Self {
    TimingInfoList(v)
  }
}

#[derive(Debug, Default)]
/// Wrapper over Vec<Tick>.
pub struct TickList(pub Vec<Tick>);

impl TickList {
  /// Create a TickList from a String.
  ///
  /// # Examples
  ///
  /// ```
  /// // a tick list with two ticks
  /// let tick_list = TickList::from_string(String::from("a:0@120,b:0@120"));
  /// ```
  pub fn from_string(s: String) -> Result<Self, anyhow::Error> {
    let mut v: Vec<Tick> = vec![];
    let ticks: Vec<&str> = s.split(',').collect();
    for tick in ticks {
      let tick = Tick::from_string(tick.to_string())?;
      v.push(tick);
    }
    Ok(TickList(v))
  }
  pub fn get_timing_info_list(
    &self,
    bpms: BpmList,
    divisors: DivisorList,
    start_time: Duration,
    music_time: Duration,
    travel_time: Duration,
    ratemod: f32,
  ) -> TimingInfoList {
    let ticks = &self.0;
    let mut v: Vec<TimingInfo> = vec![];
    // first tick
    let bpm = bpms.at_tick(0);
    let divisor = divisors.at_tick(0);
    // FIXME
    v.push(TimingInfo {
      instance_time: start_time,
      hit_time: music_time,
      end_time: music_time + ticks[0].duration(bpm.value, divisor.value, ratemod),
    });
    // rest of the ticks
    for (i, tick) in &mut ticks[1..].iter().enumerate() {
      let prev_timing_info = v.last().unwrap();
      let bpm = bpms.at_tick(i as u32);
      let divisor = divisors.at_tick(i as u32);
      // FIXME
      v.push(TimingInfo {
        instance_time: prev_timing_info.end_time - travel_time,
        hit_time: prev_timing_info.end_time,
        end_time: prev_timing_info.end_time + tick.duration(bpm.value, divisor.value, ratemod),
      });
    }
    TimingInfoList::new(v)
  }
}

#[derive(Debug)]
pub struct Chart {
  pub metadata: Metadata,
  pub audio: Sound,
  pub ticks: TickList,
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
    let Some(ticks) = map.get("ticks") else { return Err(ChartError::MissingKeyValuePair(String::from("ticks")).into()); };
    let ticks = ticks.to_string();
    // bpms
    let bpms = match (map.get("bpm"), map.get("bpms")) {
      (Some(bpm), None) => BpmList::from_f64(bpm.parse::<f64>()?),
      (None, Some(bpms)) => BpmList::from_string(bpms.to_string())?,
      (Some(_), Some(_)) => { return Err(ChartError::ConflictingBpmInformation.into()); },
      (None, None) => { return Err(ChartError::MissingBpmInformation.into()); },
    };
    // divisor(s)
    let divisors = match (map.get("divisor"), map.get("divisors")) {
      (Some(divisor), None) => DivisorList::from_u8(divisor.parse::<u8>()?),
      (None, Some(divisors)) => DivisorList::from_string(divisors.to_string())?,
      (Some(_), Some(_)) => { return Err(ChartError::ConflictingDivisorInformation.into()); },
      (None, None) => { return Err(ChartError::MissingDivisorInformation.into()); },
    };
    // metadata
    info!("creating metadata...");
    let metadata = Metadata::new(
      version,
      title.clone(),
      subtitle.clone(),
      artist.clone(),
      credit,
      bpms,
      divisors,
    );
    // audio
    info!("creating audio object...");
    let audio_filename = match subtitle.as_str() {
      "" => format!("{} - {}.wav", artist, title),
      _ => format!("{} - {} ({}).wav", artist, title, subtitle),
    };
    let audio = Sound::new(&audio_filename);
    // ticks
    info!("creating tick list...");
    let ticks = TickList::from_string(ticks)?;
    info!("parsed {} ticks", ticks.0.len());
    info!("finished!");
    // chart
    let chart = Chart {
      metadata,
      audio,
      ticks,
    };
    // info!("{:?}", chart);
    Ok(chart)
  }
  /// An empty chart.
  /// This is used internally when there is no chart being played.
  pub fn empty() -> Self {
    Chart {
      metadata: Metadata::default(),
      audio: Sound::empty(),
      ticks: TickList::default(),
    }
  }
  /// Begin playing this chart.
  pub fn play(&self, ratemod: f32) {
    let Metadata { title, subtitle, artist, credit, .. } = &self.metadata;
    // info!("playing chart \"{} - {}\" (mapped by {}) at {}bpm ({}x)...", artist, title, credit, starting_bpm, ratemod);
    match subtitle.as_str() {
      "" => info!("playing chart \"{} - {}\" (mapped by {})...", artist, title, credit),
      _ => info!("playing chart \"{} - {} ({})\" (mapped by {})...", artist, title, subtitle, credit),
    };
    self.audio.set_speed(ratemod);
    // self.audio.play();
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
  #[error("found conflicting divisor information")]
  ConflictingDivisorInformation,
  #[error("missing divisor information")]
  MissingDivisorInformation,
}

/// Status of the currently active chart.
// #[derive(Default)]
pub enum ChartStatus {
  // Countdown,
  // #[default]
  None,
  Paused,
  Playing,
}

/// Info for KhelState about the currently active chart.
pub struct ChartInfo {
  pub chart: Chart,
  pub status: ChartStatus,
  pub start_time: Duration,
  pub music_time: Duration,
  pub instance_tick: u32,
  pub hit_tick: u32,
  pub end_tick: u32,
}

impl ChartInfo {
  pub fn new(chart: Chart) -> Self {
    ChartInfo {
      chart,
      status: ChartStatus::None,
      start_time: Duration::ZERO,
      music_time: Duration::ZERO,
      instance_tick: 0,
      hit_tick: 0,
      end_tick: 0,
    }
  }
  pub fn none() -> Self {
    ChartInfo {
      chart: Chart::empty(),
      status: ChartStatus::None,
      start_time: Duration::MAX,
      music_time: Duration::MAX,
      instance_tick: 0,
      hit_tick: 0,
      end_tick: 0,
    }
  }
}

/// Serialize a key-value pair from a (String, String) into .khel format.
// fn serialize_kv(_raw: (String, String)) -> Result<String, anyhow::Error> {
//   // TODO
//   Ok(String::new())
// }

/// Deserialize a key-value pair from .khel format into a (String, String).
fn deserialize_kv(raw: String) -> Result<(String, String), anyhow::Error> {
  let equals = raw.find('=');
  let Some(equals) = equals else { panic!("malformed key-value pair"); };
  let key = &raw[0..equals];
  if !raw.ends_with(';') {
    panic!("malformed key-value pair: {}", key);
  }
  let value = &raw[equals+1..raw.len()-1];
  Ok((key.to_string(), value.to_string()))
}
