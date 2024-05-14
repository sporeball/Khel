use khel::App;
use winit::event_loop::{ControlFlow, EventLoop};

fn main() -> Result<(), anyhow::Error> {
  env_logger::init();

  let mut app = App::default();
  // let window = app.window.as_ref().unwrap();
  // let state = KhelState::new();
  let event_loop = EventLoop::new().unwrap();
  event_loop.set_control_flow(ControlFlow::Poll);
  event_loop.run_app(&mut app)?;

  Ok(())
}
