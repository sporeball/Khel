use std::fs::File;
use std::io::BufReader;
// use log::info;
use rodio::{Decoder, OutputStream, OutputStreamHandle, Sink};

pub struct Sound {
  pub stream: OutputStream,
  pub stream_handle: OutputStreamHandle,
  pub sink: Sink,
}

impl Sound {
  pub fn new(filename: &str) -> Self {
    let path = std::path::Path::new(env!("OUT_DIR"))
      .join("assets")
      .join(filename);
    let audio_file = BufReader::new(File::open(path).unwrap());
    let (stream, stream_handle) = OutputStream::try_default().unwrap();
    let sink = Sink::try_new(&stream_handle).unwrap();
    let source = Decoder::new(audio_file).unwrap();
    sink.append(source);
    Self {
      stream,
      stream_handle,
      sink,
    }
  }
  pub fn play(&self) {
    self.sink.play();
  }
}