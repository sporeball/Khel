use khel::{KhelState, texture};
use winit::{event::{Event, WindowEvent}, event_loop::{ControlFlow, EventLoop}, window::WindowBuilder};

fn main() -> Result<(), anyhow::Error> {
  env_logger::init();
  // let mut app = App::default();
  let event_loop = EventLoop::new().unwrap();
  event_loop.set_control_flow(ControlFlow::Poll);
  let window = WindowBuilder::new().build(&event_loop).unwrap();
  let mut state = KhelState::new(window);
  let device = &state.device;
  // objects
  let objects = &mut state.objects;
  let [
    circle_red,
    circle_green,
  ] = objects.as_mut_slice() else { todo!(); };
  circle_red.instantiate(0.9, 0.9, device);
  circle_green.instantiate(0.0, 0.0, device);
  // run event loop
  // event_loop.run_app(&mut app)?;
  let _ = event_loop.run(move |event, elwt| {
    // let Some(ref mut state) = state else { todo!(); };
    match event {
      Event::WindowEvent { event, .. } => {
        if !state.input(&event) {
          match event {
            WindowEvent::CloseRequested => {
              elwt.exit();
            },
            WindowEvent::RedrawRequested => {
              // let Some(ref mut state) = state else { todo!(); };
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
              // let Some(ref mut state) = state else { todo!(); };
              state.resize(physical_size);
              // state.diffuse_texture.vertex_buffer = texture::create_vertex_buffer(state.diffuse_texture.texture.size(), physical_size, &state.device);
              for object in &mut state.objects {
                object.texture.vertex_buffer = texture::create_vertex_buffer(object.texture.texture.size(), physical_size, &state.device);
              }
            },
            WindowEvent::ScaleFactorChanged { scale_factor: _, inner_size_writer: _ } => todo!(),
            _ => (),
          }
          state.egui.handle_input(&mut state.window, &event);
        }
      },
      _ => {},
    }
  });

  Ok(())
}
