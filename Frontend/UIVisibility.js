"use strict";

/* Functions for fading UI elements in and out */

const UIVisibilityState = Object.freeze({"invisible":0, "appearing":1, "visible":2, "disappearing": 3})

// let example_vis_state = {
// 	visible: UIVisibilityState.invisible,
// 	element: $('my_form_id')
// }

// Set element hidden attribute in HTML if it starts invisible
function createUIVisibilityStateObject(element, visible=false, maxOpacity=1.0) {
	if(visible) {
		element.style.opacity = maxOpacity
		return {
			visible: UIVisibilityState.visible,
			element: element,
			maxOpacity: maxOpacity
		}
	}
	element.style.opacity = 0.0
	return {
		visible: UIVisibilityState.invisible,
		element: element,
		maxOpacity: maxOpacity
	}
}

function fadeInElement(visibilityState, callback=null) {
	if(visibilityState.visible == UIVisibilityState.visible) return

	if (typeof window.Velocity == 'undefined') {
		visibilityState.element.style.opacity = visibilityState.maxOpacity
		visibilityState.element.hidden = false
		visibilityState.visible = UIVisibilityState.visible
		if(callback != null) {
			callback()
		}
	}
	else {
		visibilityState.visible = UIVisibilityState.appearing
		visibilityState.element.style.opacity = 0.0
		visibilityState.element.hidden = false
		Velocity(visibilityState.element, { opacity: visibilityState.maxOpacity }, {duration:80, complete: function(){
			visibilityState.visible = UIVisibilityState.visible

			if(callback != null) {
				callback()
			}
		}});
	}
}

function fadeOutElement(visibilityState, callback=null) {
	if(visibilityState.visible == UIVisibilityState.invisible) return

	if (typeof window.Velocity == 'undefined') {
		visibilityState.element.style.opacity = 0.0
		visibilityState.element.hidden = true
		visibilityState.visible = UIVisibilityState.invisible
		if(callback != null) {
			callback()
		}
	}
	else {
		visibilityState.visible = UIVisibilityState.disappearing
		visibilityState.element.style.opacity = visibilityState.maxOpacity
		Velocity(visibilityState.element, { opacity: 0.0 }, {duration:80, complete: function(){
			visibilityState.visible = UIVisibilityState.invisible	
			visibilityState.element.hidden = true

			if(callback != null) {
				callback()
			}
		}});
	}

	
}

function fadeInOrOutElement(visibilityState, callback=null) {
	if(visibilityState.visible == UIVisibilityState.invisible) {
		fadeInElement(visibilityState, callback)
	}
	else if(visibilityState.visible == UIVisibilityState.visible) {
		fadeOutElement(visibilityState, callback)
	}
}

