use crate::{KhelState, chart::{Chart, ChartInfo, ChartStatus}};
use std::time::Duration;
use egui::{epaint::Shadow, Button, Color32, Context, Frame, Label, Margin, Rounding};
use egui_wgpu::Renderer;
use log::{info, warn};
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
  let ctx = state.egui.context.to_owned();
  // egui::Window::new("Khel")
  egui::CentralPanel::default()
    .frame(Frame {
      outer_margin: Margin::same(10.0),
      rounding: Rounding::ZERO,
      shadow: Shadow::NONE,
      fill: Color32::TRANSPARENT,
      // stroke: Stroke::NONE,
      ..Default::default()
    })
    .show(&ctx, |ui| {
      ui.add(Label::new(format!("{:.0} fps", state.fps.avg())));
      ui.end_row();
      if ui.add(Button::new("Load chart 1")).clicked() {
        let Ok(chart) = Chart::read_from_disk("charts/hyperpops 2023.khel") else { return };
        state.chart_info = ChartInfo::new(chart);
      }
      if ui.add(Button::new("Load chart 2")).clicked() {
        let Ok(chart) = Chart::read_from_disk("charts/amanita.khel") else { return };
        state.chart_info = ChartInfo::new(chart);
      }
      if ui.add(Button::new("Play chart")).clicked() {
        let chart_info = &mut state.chart_info;
        if let ChartStatus::Playing = chart_info.status {
          warn!("chart is already playing");
          return;
        }
        let chart = &chart_info.chart;
        let ticks = &chart.ticks.0;
        let Some(first_tick) = ticks.get(0) else {
          warn!("could not play chart, is it empty?");
          return;
        };
        let start_bpm = first_tick.bpm as f64;
        let one_beat = Duration::from_secs_f64(60f64 / start_bpm);
        let one_bar = one_beat * 4;
        // set chart start time
        // this will play the chart once the time has passed
        let start_time = state.time + one_bar;
        chart_info.status = ChartStatus::Countdown;
        chart_info.start_time = start_time;
        // set tick info
        let tick_info = chart.ticks.get_tick_info(chart.metadata.divisor, start_time);
        state.tick_info = Some(tick_info);
        // info!("{:?}", state.tick_info);
      }
      ui.end_row();
    });
}
