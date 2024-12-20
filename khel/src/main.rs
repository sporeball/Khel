use clap::Parser;
use game_loop::game_loop;
use khel::KhelState;
use log::info;
use std::sync::Arc;
use std::time::Instant;
use winit::{event::Event, event_loop::{ControlFlow, EventLoop}, window::WindowBuilder};

/// Colorful rhythm game
#[derive(Parser)]
struct Args {
  /// Run Khel using secondary graphics backend
  #[clap(long, action)]
  gl: bool,
}

fn main() -> Result<(), anyhow::Error> {
  env_logger::init();
  let args = Args::parse();
  // let mut app = App::default();
  let event_loop = EventLoop::new().unwrap();
  event_loop.set_control_flow(ControlFlow::Poll);
  let window = WindowBuilder::new().build(&event_loop).unwrap();

  let window = Arc::new(window);
  info!("created window (inner size = {:?})", window.inner_size());
  let mut state = KhelState::new(window.clone(), args.gl);

  state.objects.instantiate(
    "line_white",
    -1.0,
    0.0,
    state.size,
    &state.device,
    &state.queue,
  );

  state.groups.insert("pure_calculation".to_string(), vec![]);
  state.groups.insert("hit_objects".to_string(), vec![]);
  state.groups.insert("hits_and_holds".to_string(), vec![]);
  state.groups.insert("timing_lines".to_string(), vec![]);

  let now = Instant::now();

  game_loop(
    event_loop,
    window,
    state,
    1000,
    0.1,
    move |g| {
      g.game.time = now.elapsed().as_secs_f64();
      g.game.update();
    },
    |g| {
      let _ = g.game.render();
      g.game.fps.tick();
    },
    |g, event| {
      // match event {
      //   Event::WindowEvent { event, .. } => {
      //     if !g.game.input(event) { g.exit(); }
      //   },
      //   _ => {},
      // }
      if let Event::WindowEvent { event, .. } = event {
        if !g.game.input(event) { g.exit(); }
      }
    }
  ).unwrap();

  Ok(())
}
