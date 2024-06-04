use khel::{KhelState, texture};
use winit::{event::{Event, WindowEvent}, event_loop::{ControlFlow, EventLoop}, window::WindowBuilder};

fn main() -> Result<(), anyhow::Error> {
  env_logger::init();
  // let mut app = App::default();
  let event_loop = EventLoop::new().unwrap();
  event_loop.set_control_flow(ControlFlow::Poll);
  let window = WindowBuilder::new().build(&event_loop).unwrap();
  let mut state = Some(KhelState::new(window));
  let Some(ref mut state) = state else { todo!(); };
  state.instantiate("circle_red", 0.9, 0.9);
  state.instantiate("circle_green", 0.0, 0.0);
  let to_be_destroyed = state.instantiate("circle_red", 0.0, 0.0);
  state.destroy(to_be_destroyed);

  // run event loop
  // event_loop.run_app(&mut app)?;
  let _ = event_loop.run(move |event, elwt| {
    // handle event
    match event {
      Event::WindowEvent { event, .. } => {
        // state.input gets a chance to handle the event instead
        // any events it does not handle return false
        if !state.input(&event) {
          match event {
            WindowEvent::CloseRequested => {
              elwt.exit();
            },
            WindowEvent::RedrawRequested => {
              state.window.request_redraw();
              state.update();
              match state.render() {
                Ok(_) => {},
                // reconfigure the surface if lost
                Err(wgpu::SurfaceError::Lost) => state.resize(state.size),
                // the system is out of memory, we should probably quit
                Err(wgpu::SurfaceError::OutOfMemory) => elwt.exit(),
                // all other errors (Outdated, Timeout) should be resolved by the next frame
                Err(e) => eprintln!("{:?}", e),
              }
            },
            WindowEvent::Resized(physical_size) => {
              state.resize(physical_size);
              for object in state.objects.values_mut() {
                object.texture.vertex_buffer = texture::create_vertex_buffer(object.texture.texture.size(), physical_size, &state.device);
              }
            },
            WindowEvent::ScaleFactorChanged { scale_factor: _, inner_size_writer: _ } => todo!(),
            _ => (),
          }
          // egui
          state.egui.handle_input(&mut state.window, &event);
        }
      },
      _ => {},
    }
  });

  Ok(())
}
