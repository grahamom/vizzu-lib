import { data } from '../../../test_data/chart_types_eu.mjs'

const testSteps = [
  (chart) =>
    chart.animate({
      data,
      config: {
        channels: {
          x: 'Year',
          y: 'Value 5 (+/-)',
          label: 'Value 5 (+/-)'
        },
        title: 'Histogram with (-) Nums'
      }
    })
]

export default testSteps
