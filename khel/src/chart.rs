use crate::{read_lines, sound::Sound};
use std::collections::HashMap;
use std::path::Path;
use std::time::Duration;
use itertools::Itertools;
use log::info;

pub const CHART_VERSION: u8 = 0;

#[derive(Debug, Default)]
pub struct Metadata {
  pub version: u8,
  pub title: String,
  pub subtitle: String,
  pub artist: String,
  pub credit: String,
  // TODO: divisor change
  pub divisor: u8,
}

impl Metadata {
  pub fn new(
    version: u8,
    title: String,
    subtitle: String,
    artist: String,
    credit: String,
    divisor: u8,
  ) -> Self {
    Metadata {
      version,
      title,
      subtitle,
      artist,
      credit,
      divisor,
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
    column(self.keys[0]).unwrap()
  }
  /// Return the asset that should be used to render this HitObject.
  pub fn asset(&self) -> &str {
    let rows: u8 = self.keys
      .iter()
      .map(|&c| row(c).expect("found invalid key in hit object"))
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
  pub fn from_string(s: String) -> Self {
    let s: Vec<&str> = s.split("/").collect();
    let mut v: Vec<HitObject> = vec![];
    let hits: Vec<String> = s.get(0).unwrap_or(&"").split("+").map(String::from).collect();
    let holds: Vec<String> = s.get(1).unwrap_or(&"").split("+").map(String::from).collect();
    // hits
    for hit in hits.iter() {
      if hit.is_empty() {
        continue;
      }
      if !hit.chars().all_unique() {
        panic!("found duplicate hit char");
      }
      if !hit.chars().map(column).all_equal() {
        panic!("attempted to create hit across multiple columns");
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
        panic!("found duplicate hold char");
      }
      if !hold.chars().map(column).all_equal() {
        panic!("attempted to create hold across multiple columns");
      }
      v.push(HitObject::from_keys(
        hold.chars().collect(),
        HitObjectType::Hold
      ));
    }
    HitObjectList(v)
  }
}

// #[derive(Debug, Default)]
#[derive(Clone, Debug)]
pub struct Tick {
  pub length: u8,
  pub bpm: u16,
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
      panic!("attempted to create tick from an empty string");
    }
    let v: Vec<&str> = s.split(&[':', '@']).collect();
    if v.len() > 3 {
      panic!("attempted to create tick with too many parts");
    }
    let head = v.get(0).unwrap();
    let length = v.get(1).expect("missing tick length").parse::<u8>()?;
    let bpm = v.get(2).expect("missing tick bpm").parse::<u16>()?;
    let hit_objects = HitObjectList::from_string(head.to_string());
    let tick = Tick {
      length,
      bpm,
      hit_objects,
    };
    Ok(tick)
  }
  /// Return the length of this Tick as a Duration.
  pub fn duration(&self, divisor: u8) -> Duration {
    let divisor = divisor as f64;
    // 1-256
    let length = (self.length + 1) as f64;
    let one_bar = Duration::from_secs_f64((60f64 / self.bpm as f64) * 4.0);
    one_bar.div_f64(divisor).mul_f64(length)
  }
  /// Return the asset that should be used to render this tick's timing line.
  pub fn timing_line_asset(&self, divisor: u8, units_elapsed: u32) -> &str {
    match divisor {
      1 | 2 | 4 => "line_red",
      6 => match units_elapsed % 6 {
        0 | 3 => "line_red",
        _ => "line_magenta",
      },
      8 => match units_elapsed % 2 {
        0 => "line_red",
        _ => "line_blue",
      },
      12 => match units_elapsed % 12 {
        0 | 3 | 6 | 9 => "line_red",
        _ => "line_magenta",
      }
      16 => match units_elapsed % 4 {
        0 => "line_red",
        2 => "line_blue",
        _ => "line_yellow",
      },
      24 => match units_elapsed % 24 {
        0 | 6 | 12 | 18 => "line_red",
        3 | 9 | 15 | 21 => "line_magenta",
        _ => "line_cyan",
      },
      32 => match units_elapsed % 8 {
        0 => "line_red",
        4 => "line_blue",
        2 | 6 => "line_yellow",
        _ => "line_green",
      },
      _ => panic!("unsupported divisor"),
    }
  }
}

#[derive(Debug)]
/// Info for KhelState about the current tick.
pub struct TickInfo {
  pub instance_time: Duration,
  pub hit_time: Duration,
  pub end_time: Duration,
}

// pub struct TickInfoList(pub Vec<TickInfo>);

// impl TickInfoList {
// }

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
    let ticks: Vec<&str> = s.split(",").collect();
    for tick in ticks {
      let tick = Tick::from_string(tick.to_string())?;
      v.push(tick);
    }
    Ok(TickList(v))
  }
  pub fn get_tick_info(
    &self,
    divisor: u8,
    start_time: Duration,
  ) -> Vec<TickInfo> {
    let ticks = &self.0;
    let mut tick_info: Vec<TickInfo> = vec![];
    // the first tick is known
    let one_bar = Duration::from_secs_f64((60f64 / ticks[0].bpm as f64) * 4.0);
    tick_info.push(TickInfo {
      instance_time: start_time - one_bar,
      hit_time: start_time,
      end_time: start_time + ticks[0].duration(divisor),
    });
    for (i, tick) in &mut ticks[1..].iter().enumerate() {
      let last_tick_info = tick_info.last().unwrap();
      let one_bar = Duration::from_secs_f64((60f64 / ticks[i].bpm as f64) * 4.0);
      tick_info.push(TickInfo {
        instance_time: last_tick_info.end_time - one_bar,
        hit_time: last_tick_info.end_time,
        end_time: last_tick_info.end_time + tick.duration(divisor),
      });
    }
    tick_info
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
    for line in lines.flatten() { // maps Result<String> to String
      if line.is_empty() {
        continue;
      }
      let (key, value) = deserialize_kv(line)?;
      map.insert(key, value);
    }
    info!("found {} key-value pairs", map.keys().len());
    // required key-value pairs
    let version = map.get("version").expect("missing key-value pair: version").parse::<u8>()?;
    let title = map.get("title").expect("missing key-value pair: title").to_string();
    let subtitle = map.get("subtitle").expect("missing key-value pair: subtitle").to_string();
    let artist = map.get("artist").expect("missing key-value pair: artist").to_string();
    let credit = map.get("credit").expect("missing key-value pair: credit").to_string();
    let divisor = map.get("divisor").expect("missing key-value pair: divisor").parse::<u8>()?;
    let ticks = map.get("ticks").expect("missing key-value pair: ticks").to_string();
    // metadata
    info!("creating metadata...");
    let metadata = Metadata::new(version, title.clone(), subtitle.clone(), artist.clone(), credit, divisor);
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
  pub fn play(&self) -> () {
    let Metadata { title, artist, credit, .. } = &self.metadata;
    let ticks = &self.ticks.0;
    let starting_bpm = ticks[0].bpm as f64;
    info!("playing chart \"{} - {}\" (mapped by {}) at {}bpm...", artist, title, credit, starting_bpm);
    self.audio.play();
  }
}

/// Status of the currently active chart.
// #[derive(Default)]
pub enum ChartStatus {
  Countdown,
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
  pub tick: u32,
  pub units_elapsed: u32,
}

impl ChartInfo {
  pub fn new(chart: Chart) -> Self {
    ChartInfo {
      chart,
      status: ChartStatus::None,
      start_time: Duration::ZERO,
      tick: 0,
      units_elapsed: 0,
    }
  }
  pub fn none() -> Self {
    ChartInfo {
      chart: Chart::empty(),
      status: ChartStatus::None,
      start_time: Duration::MAX,
      tick: 0,
      units_elapsed: 0,
    }
  }
}

/// Return the column that a key is in.
fn column(c: char) -> Option<u8> {
  match c {
    'q' | 'a' | 'z' => Some(0),
    'w' | 's' | 'x' => Some(1),
    'e' | 'd' | 'c' => Some(2),
    'r' | 'f' | 'v' => Some(3),
    't' | 'g' | 'b' => Some(4),
    'y' | 'h' | 'n' => Some(5),
    'u' | 'j' | 'm' => Some(6),
    'i' | 'k' | ',' => Some(7),
    'o' | 'l' | '.' => Some(8),
    'p' | ';' | '/' => Some(9),
    _ => None,
  }
}

/// Return the row that a key is in.
fn row(c: char) -> Option<u8> {
  match c {
    'q' | 'w' | 'e' | 'r' | 't' | 'y' | 'u' | 'i' | 'o' | 'p' => Some(1),
    'a' | 's' | 'd' | 'f' | 'g' | 'h' | 'j' | 'k' | 'l' | ';' => Some(2),
    'z' | 'x' | 'c' | 'v' | 'b' | 'n' | 'm' | ',' | '.' | '/' => Some(4),
    _ => None,
  }
}

/// Serialize a key-value pair from a (String, String) into .khel format.
fn serialize_kv(_raw: (String, String)) -> Result<String, anyhow::Error> {
  // TODO
  Ok(String::new())
}

/// Deserialize a key-value pair from .khel format into a (String, String).
fn deserialize_kv(raw: String) -> Result<(String, String), anyhow::Error> {
  let equals = raw.find("=");
  let Some(equals) = equals else { panic!("malformed key-value pair"); };
  let key = &raw[0..equals];
  if !raw.ends_with(";") {
    panic!("malformed key-value pair: {}", key);
  }
  let value = &raw[equals+1..raw.len()-1];
  Ok((key.to_string(), value.to_string()))
}
