import { data } from '../../../test_data/tutorial.mjs'

const testSteps = [
	(chart) =>
		chart.animate({
			data,
			config: {
				channels: {
					x: { attach: ['Values 2'] },
					y: { attach: ['Values 1'], range: { min: '0%', max: '110%' } },
					color: { attach: ['Categ. Parent'] },
					size: { attach: ['Values 3'] },
					label: { attach: ['Categ. Child', 'Values 1'] }
				},
				title: 'Operations: Circle - Merged.',
				geometry: 'circle'
			}
		}),
	(chart) =>
		chart.animate({
			config: {
				channels: {
					y: { attach: ['Categ. Parent'], split: true }
				},
				title: 'Operations: Circle - Splitted.',
				orientation: 'horizontal'
			}
		}),
	(chart) =>
		chart.animate({
			config: {
				channels: {
					y: { detach: ['Categ. Parent'], split: false }
				},
				title: 'Operations: Circle - Merged.'
			}
		})
]

export default testSteps
