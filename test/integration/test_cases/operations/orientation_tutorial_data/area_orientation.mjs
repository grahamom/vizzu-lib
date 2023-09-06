import { data } from "../../../test_data/tutorial.mjs";

const testSteps = [
  (chart) =>
    chart.animate({
      data: data,
      config: {
        channels: {
          x: { attach: ["Timeseries"] },
          y: {
            attach: ["Values 1", "Categ. Parent"],
            range: { min: "0%", max: "110%" },
          },
          color: { attach: ["Categ. Parent"] },
          label: { attach: ["Values 1"] },
        },
        title: "Operations: Area - Orientation Change - No:1.",
        geometry: "area",
      },
    }),
  (chart) =>
    chart.animate({
      config: {
        channels: {
          y: { detach: ["Values 1"] },
          x: { attach: ["Values 1"] },
          color: { detach: ["Categ. Parent"], attach: ["Timeseries"] },
        },
        title: "Operations: Area - Orientation Change - No:2",
      },
    }),
  (chart) =>
    chart.animate({
      config: {
        channels: {
          x: { detach: ["Values 1"] },
          y: { attach: ["Values 1"] },
          color: { detach: ["Timeseries"], attach: ["Categ. Parent"] },
        },
        title: "Operations: Area - Orientation Change - No:3",
      },
    }),
];

export default testSteps;
