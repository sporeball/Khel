use crate::{read_lines, sound::Sound};
use std::collections::HashMap;
use std::fs::File;
use std::path::Path;
use std::time::Duration;
use itertools::Itertools;
use log::info;

pub const CHART_VERSION: u8 = 0;

#[derive(Debug)]
pub struct Metadata {
  pub version: u8,
  pub title: String,
  pub subtitle: String,
  pub artist: String,
  pub credit: String,
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

#[derive(Debug)]
pub enum HitObjectType {
  Hit,
  Hold,
}

#[derive(Debug)]
pub struct HitObject {
  pub t: HitObjectType,
  pub keys: Vec<String>,
}

impl HitObject {
  pub fn from_keys(keys: Vec<String>, t: HitObjectType) -> Self {
    HitObject {
      t,
      keys,
    }
  }
}

#[derive(Debug)]
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
        hit.chars().map(String::from).collect(),
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
        hold.chars().map(String::from).collect(),
        HitObjectType::Hold
      ));
    }
    HitObjectList(v)
  }
}

#[derive(Debug)]
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
  /// // a tick with one HitObject lasting 4 units at 120bpm
  /// let tick = Tick::from_string(String::from("a:0@120"));
  /// ```
  pub fn from_string(s: String) -> Result<Self, anyhow::Error> {
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
}

#[derive(Debug)]
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
}

#[derive(Debug)]
pub struct Chart {
  pub metadata: Metadata,
  pub audio: Sound,
  pub ticks: TickList,
}

impl Chart {
  /// Write this chart to disk as a .khel file.
  pub fn write_to_disk(&self, filename: String) -> Result<(), anyhow::Error> {
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
  /// Begin playing this chart.
  pub fn play(&self) -> () {
    let Metadata { title, artist, credit, .. } = &self.metadata;
    let ticks = &self.ticks.0;
    let starting_bpm = ticks[0].bpm as f64;
    info!("playing chart \"{} - {}\" (mapped by {}) at {}bpm...", artist, title, credit, starting_bpm);
    self.audio.play();
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

/// Serialize a key-value pair from a (String, String) into .khel format.
fn serialize_kv(raw: (String, String)) -> Result<String, anyhow::Error> {
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
