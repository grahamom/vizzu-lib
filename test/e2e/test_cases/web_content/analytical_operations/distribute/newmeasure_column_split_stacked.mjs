import { data_4 } from '../../../../test_data/chart_types_eu.mjs'

const description = `- move the Dimension from the X-axis & replace the Measure with it on the Label channel
- remove the Dimension from the Y-axis
- add a new Measure to the X-axis
- switch the Geometry from Rectangle (default) to Circe
- set the Split parameter from true to false`
const testSteps = [
	(chart) =>
		chart.animate({
			data: data_4,

			config: {
				channels: {
					x: 'Year',
					y: { set: ['Country', 'Value 2 (+)'], split: true },
					color: 'Country',
					label: 'Value 2 (+)'
				}
			},
			style: {
				plot: {
					marker: {
						label: {
							position: 'top',
							fontSize: '0.6em'
						}
					}
				}
			}
		}),

	(chart) =>
		chart.animate({
			config: {
				channels: {
					x: 'Value 5 (+/-)',
					y: { set: 'Value 2 (+)', split: false },
					label: 'Year'
				},

				geometry: 'circle'
			}
		}),
	(chart) => {
		chart.feature('tooltip', true)
		return chart
	}
]

export { description }
export default testSteps
