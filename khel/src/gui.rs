use crate::{KhelState, chart::{Chart, ChartInfo, ChartStatus}};
use egui::{epaint::Shadow, style::{Spacing, Style}, Button, Color32, Context, Frame, Label, Margin, RichText, Rounding, Slider, TextEdit, Vec2};
use egui_wgpu::Renderer;
use log::error;
use log::info;
// use log::warn;
use wgpu::{Device, TextureFormat};
use winit::{event::WindowEvent, window::Window};

pub struct EguiRenderer {
  pub context: Context,
  pub state: egui_winit::State,
  pub renderer: Renderer,
}

impl EguiRenderer {
  pub fn new(
    device: &Device,
    output_color_format: TextureFormat,
    output_depth_format: Option<TextureFormat>,
    msaa_samples: u32,
    window: &Window,
  ) -> Self {
    let context = Context::default();
    let id = context.viewport_id();
    let state = egui_winit::State::new(context.clone(), id, &window, None, None);
    let renderer = Renderer::new(
      device,
      output_color_format,
      output_depth_format,
      msaa_samples,
    );
    Self {
      context,
      state,
      renderer,
    }
  }
  pub fn handle_input(&mut self, window: &Window, event: &WindowEvent) {
    let _ = self.state.on_window_event(window, event);
  }
}

/// Create a CentralPanel with some kind of GUI inside.
/// This function is defined outside EguiRenderer so that the closure passed
/// to CentralPanel::show can access the methods on KhelState directly.
pub fn gui(state: &mut KhelState) {
  let ctx = &state.egui.context.clone();
  // TODO: parameterize
  ctx.set_style(Style {
    spacing: Spacing {
      item_spacing: Vec2 { x: 2.0, y: 4.0 },
      ..Default::default()
    },
    ..Default::default()
  });
  egui::CentralPanel::default()
    .frame(Frame {
      outer_margin: Margin::same(10.0),
      rounding: Rounding::ZERO,
      shadow: Shadow::NONE,
      fill: Color32::TRANSPARENT,
      // stroke: Stroke::NONE,
      ..Default::default()
    })
    .show(ctx, |ui| {
      ui.add(Label::new(format!("{:.0} fps", state.fps.avg())));
      ui.add(TextEdit::singleline(&mut state.chart_path).hint_text("Chart"));
      if ui.add(Button::new("Play")).clicked() {
        let chart = match Chart::read_from_disk(&state.chart_path) {
          Ok(chart) => {
            state.error = None;
            chart
          },
          Err(chart_error) => {
            error!("{}", chart_error);
            state.error = Some(chart_error);
            return;
          }
        };
        state.chart_info = ChartInfo::new(chart);
        // need to do this to stop egui from getting anxious
        if state.chart_info.chart.metadata.bpms.0.is_empty() {
          return;
        }
        state.chart_info.start_time = state.time;
        for hit_object in state.chart_info.chart.hit_objects.0.clone().iter() {
          // instantiate all of the timing lines in the chart
          // TODO
          // instantiate all of the objects in the chart
          let id = state.instantiate(
            hit_object.asset(),
            hit_object.lane_x(),
            -1.5 // dummy value, gets updated after the fact
          );
          state.groups.insert_into_group("hit_objects".to_string(), id);
        }
        // update bpms based on ratemod
        state.chart_info.chart.set_ratemod(state.ratemod);
        // start playing the chart
        state.chart_info.chart.play(state.ratemod); // does not immediately play the audio
        state.chart_info.status = ChartStatus::Playing;
      }
      // ui.add_enabled(
      //   !matches!(state.chart_info.status, ChartStatus::Playing),
      //   Slider::new(&mut state.xmod, 1.0..=6.0).step_by(0.05).text("Xmod")
      // );
      ui.add_enabled(
        !matches!(state.chart_info.status, ChartStatus::Playing),
        Slider::new(&mut state.av.value, 300.0..=1000.0).step_by(1.0).text("AV")
        // egui::DragValue::new(&mut state.av.value).speed(1.0).clamp_range(300..=1000)
      );
      ui.add_enabled(
        !matches!(state.chart_info.status, ChartStatus::Playing),
        Slider::new(&mut state.ratemod, 0.5..=2.0).step_by(0.1).text("Ratemod")
      );
      if state.error.is_some() {
        ui.add(Label::new(RichText::new(format!("error: {}", state.error.as_ref().unwrap())).color(Color32::RED)));
      }
      ui.add_visible_ui(
        matches!(state.chart_info.status, ChartStatus::Playing),
        |ui| {
          let current_bpm_string = match state.chart_info.chart.metadata.bpms.0.len() {
            0 => String::new(),
            _ => {
              let one_minute = 60.0;
              let bpm_at_zero = state.chart_info.chart.metadata.bpms.at_exact_time(0.0);
              let one_beat_at_zero = one_minute / bpm_at_zero.value;
              let one_bar_at_zero = one_beat_at_zero * 4.0;
              let current_bpm = state.chart_info.chart.metadata.bpms.at_exact_time(state.time - state.chart_info.start_time - one_bar_at_zero - one_bar_at_zero).value;
              format!("{:.2}", current_bpm)
            },
          };
          ui.vertical_centered(|ui| ui.label(RichText::new(current_bpm_string).size(20.0)));
        }
      );
    });
}
