import { data_14 } from '../../../../../test_data/chart_types_eu.mjs'

const testSteps = [
  (chart) =>
    chart.animate({
      data: data_14,
      config: {
        channels: {
          x: 'Year',
          y: ['Country', 'Value 2 (+)'],
          color: 'Country'
        },
        title: 'Stacked Area Chart',
        geometry: 'area'
      }
    }),

  (chart) =>
    chart.animate({
      config: {
        channels: {
          x: null,
          y: null,
          size: ['Year', 'Value 2 (+)'],
          color: 'Joy factors'
        },
        title: 'Change CoordSys',
        geometry: 'rectangle'
      }
    })
]

export default testSteps
