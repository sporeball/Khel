use winit::{application::ApplicationHandler, event::WindowEvent, event_loop::{ActiveEventLoop, ControlFlow, EventLoop}, window::{Window, WindowId}};

#[derive(Default)]
struct App {
  window: Option<Window>,
}

impl ApplicationHandler for App {
  fn resumed(&mut self, event_loop: &ActiveEventLoop) {
    self.window = Some(event_loop.create_window(Window::default_attributes()).unwrap());
  }
  fn window_event(&mut self, event_loop: &ActiveEventLoop, _id: WindowId, event: WindowEvent) {
    match event {
      WindowEvent::CloseRequested => {
        event_loop.exit();
      },
      WindowEvent::RedrawRequested => {
        self.window.as_ref().unwrap().request_redraw();
      }
      _ => (),
    }
  }
}

fn main() -> Result<(), anyhow::Error> {
  env_logger::init();
  let event_loop = EventLoop::new().unwrap();
  // let window_attributes = Window::default_attributes()
  //   .with_title("Khel");
  event_loop.set_control_flow(ControlFlow::Poll);
  let mut app = App::default();
  event_loop.run_app(&mut app)?;

  Ok(())
}
