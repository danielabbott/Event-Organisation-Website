"use strict";


let S3_URL = 'https://da-event-org.s3.amazonaws.com/'

let pwd = $('pwd')
let pwd_confirm = $('pwd_confirm')

// Objects that track whether dropdown menus are showing or not

let logInFormVisibilityState = createUIVisibilityStateObject($('loginForm'))
let signUpFormVisibilityState = createUIVisibilityStateObject($('signUpForm'))
let pwdResetFormVisibilityState = createUIVisibilityStateObject($('passwordResetForm'))
let userDropdownVisibilityState = createUIVisibilityStateObject($('userDropdown'))
let lhsDropdownVisibilityState = createUIVisibilityStateObject($('lhsDropdownMenu'))
let notificationsDropdownVisibilityState = createUIVisibilityStateObject($('notificationsDropdown'))
let hideScreenVisibilityState = createUIVisibilityStateObject($('hideScreen'), false, 0.9)
let addCoverImageVisibilityState = createUIVisibilityStateObject($('uploadCoverImageFormArea'))
let addMediaVisibilityState = createUIVisibilityStateObject($('uploadMediaFormArea'))
let addProfilePictureVisibilityState = createUIVisibilityStateObject($('uploadProfilePictureFormArea'))

// All pages and the functions for loading the content of each page
const Pages = Object.freeze({"":loadHomePage, "user":loadUserPage, 
	"password_reset":loadPasswordResetPage, "event":loadEventPage,
	"editEvent":loadEditEventPage, "myEvents":loadMyEventsPage,
	'upcomingEvents':loadUpcomingEventsPage, 'invitations':loadInvitationsPage})

// One of the strings from Pages ^
let active_page = ''

// Hides the spinning icon
function hideLoadingSymbol() {
	$('loadingSymbol').hidden = true
}

function hideActivePage() {
	$('page').hidden = true
	$('page_' + active_page).hidden = true
	$('loadingSymbol').hidden = false
}

function showActivePage() {
	$('page').hidden = false
	$('page_' + active_page).hidden = false
	hideLoadingSymbol()
}

// e.g. 65 -> 1 hour 5 minutes
function durationToString(minutes) {
	let hrs = Math.floor(minutes/60)
	let mins = minutes%60
	return durationToString2(hrs, mins)
}

function durationToString2(hrs, mins) {
	let duration = hrs + ' hour'
	if(hrs != 1) {
		duration += 's'
	}
	duration += ' ' + mins + ' minute'
	if(mins != 1) {
		duration += 's'
	}
	return duration
}


function createEventPreviewHTML(event, showPublishedText = false) {
	let duration = durationToString(event.duration)

	let html = ''

	html += '<div class="EventPreview" id="EventPreview' + event.id + '">'

	if(event.coverImage != null) {
		html += '<img class="EventPreviewCover" src="' + S3_URL+event.coverImage + '">'
	}
	else {
		html += '<div class="EventPreviewNoCover"></div>'
	}

	html += '<div class="EventPreviewLHS"><h2 class="EventPreviewTitle">' + filterHTML(event.title) + '</h2>'
	+ '<p>' + filterHTML(event.description, false, true) + '</p></div>'
	+ '<div class="EventPreviewRHS">' + event.dateTime + '<br>' + duration

	if(showPublishedText && event.isPublished) {
		html += '<p class="RedText">Published</p>'
	}

	html += '</div></div><br><br>'
	return html
}

function createEventPreviews(events, parentObj, showPublishedText=false) {
	let html = ''

	events.forEach(event => {
		html += createEventPreviewHTML(event, showPublishedText)
	})

	parentObj.innerHTML = html

	events.forEach(event => {
		$('EventPreview' + event.id).onclick = _ => {
			switchToPage('event', {id: event.id})
		}
	})
}

function fetchHomePageEvents(name, location) {
	if(name == '') {
		name = '%'
	}
	else {
		name = '%' + name + '%'
	}
	if(location == '') {
		location = '%'
	}
	else {
		location = '%' + location + '%'
	}

	let params = ''

	if(name != '%') {
		params = '&name=' + encodeURIComponent(name)
	}
	if(location != '%') {
		params += '&location=' + encodeURIComponent(location)
	}
	params = params.substr(1)

	loadJSON('homepage_events', x => {
		createEventPreviews(x.events, $('homepage_events'))

		showActivePage()		
	}, _ => {
		hideLoadingSymbol()
		alert('Error')
	}, 'GET', params)

	
}

function loadHomePage(args) {
	fetchHomePageEvents('', '')

	let homepage_name = $('homepage_name')
	let homepage_location = $('homepage_location')

	$('homepage_filters').onsubmit = _ => {
		fetchHomePageEvents(homepage_name.value, homepage_location.value)
		return false
	}
}

function eventPageNoCoverImage(weAreOrganiser, eventID) {
	$('eventViewBanner').hidden = true
	$('eventViewBanner').src = ''
	$('deleteCoverImageButton').hidden = true
	$('uploadCoverImage').hidden = !weAreOrganiser

	$('uploadCoverImage').onclick = _ => {
		fadeInElement(hideScreenVisibilityState)
		fadeInElement(addCoverImageVisibilityState)
		loadAddCoverImageForm(eventID)
	}
}

function eventPageHasCoverImage(coverImageID, weAreOrganiser, eventID) {
	$('eventViewBanner').hidden = false
	$('eventViewBanner').src = S3_URL + coverImageID
	$('deleteCoverImageButton').hidden = !weAreOrganiser
	$('uploadCoverImage').hidden = true

	$('deleteCoverImageButton').onclick = _ => {
		if(confirm('Delete cover image?')) {
			loadFile('delete_cover_image', _ => {
				eventPageNoCoverImage(weAreOrganiser, eventID)
			}, null, 'POST', 'event_id=' + eventID)
		}
	}
}

function getMediaPreviewHTML(m, weAreOrganiser) {
	let html = '<div class="mediaContainer" id="mediaContainer' + m.id
	+ '"><img src="'
	if(m.fileName.endsWith('.png') || m.fileName.endsWith('.jpg') ||
			m.fileName.endsWith('.webp') || m.fileName.endsWith('.gif') ||
			m.fileName.endsWith('.bmp') || m.fileName.endsWith('.ico') ||
			m.fileName.endsWith('.jpeg') || m.fileName.endsWith('.svg') ||
			m.fileName.endsWith('.tif') || m.fileName.endsWith('.tiff') ||
			m.fileName.endsWith('.jfif'))
	{
		html += S3_URL+m.id
	}
	else {
		html += 'ico/file.svg'
	}
	html += '" title="' + filterHTML(m.fileName)
	+ '" id="eventMedia' + m.id
	+ '" class="eventMediaPreview">'
	+ '<br>'+ filterHTML(m.fileName)

	if(weAreOrganiser) {
		html +='<br><span class="mediaDelete" id="mediaDelete' + m.id +
		'">Delete</span>'
	}

	html += '</div>'
	return html
}

function addMediaPreviewCallbacks(m, weAreOrganiser) {
	if(!m.isCoverImage) {
		$('eventMedia' + m.id).onclick = _ => {
			window.open(S3_URL+m.id)
		}

		if(weAreOrganiser) {
			$('mediaDelete' + m.id).onclick = _ => {
				if(confirm('Delete ' + m.fileName + '?')) {
					loadFile('delete_media', _ => {
						$('mediaContainer'+m.id).remove()
					}, _ => alert('Error removing file'), 'POST', 'id='+m.id)
				}
			}
		}
	}
}

function loadEventPage(args) {
	loadJSON('event', x => {
		let editEventButton = $('editEventButton')
		let deleteEventButton = $('deleteEventButton')

		let weAreOrganiser = sessionStorage.myID != null && x.organiser_id == sessionStorage.myID

		if(weAreOrganiser) {
			editEventButton.onclick = _ => switchToPage('editEvent', {id: args.id})
			editEventButton.style.visibility = 'visible'
			deleteEventButton.style.visibility = 'visible'
			deleteEventButton.onclick = _ => {
				if(confirm("Are you sure you want to delete this event?")) {
					loadFile('delete_event', _ => switchToPage('myEvents'), null, 'POST', 'id=' + args.id)
				}
			}
		}
		else {
			editEventButton.onclick = null
			editEventButton.style.visibility = 'hidden'
			deleteEventButton.style.visibility = 'hidden'
			deleteEventButton.onclick = null
		}

		
		if(x.coverImage == null) {
			eventPageNoCoverImage(weAreOrganiser, args.id)
		}
		else {
			eventPageHasCoverImage(x.coverImage, weAreOrganiser, args.id)
		}

		$('organiserName').innerHTML = filterHTML(x.organiserName)

		$('eventName').innerHTML = filterHTML(x.name)
		$('url').innerHTML = filterHTML(x.url == null ? '' : x.url)
		if(x.public) {
			$('publicPrivateEventIcon').src = 'ico/padlock-unlock.svg'
			$('publicPrivateEvent').innerHTML = 'Public Event'
		}
		else {
			$('publicPrivateEventIcon').src = 'ico/padlock.svg'
			$('publicPrivateEvent').innerHTML = 'Private Event'
		}

		var dateSplit = x.date.split('-')
		var timeSplit = x.time.split(':')
		var startDate = new Date(dateSplit[0], dateSplit[1]-1, dateSplit[2], timeSplit[0], timeSplit[1], 0)
		var endDate = new Date(startDate.getTime() + x.durationHours*60*60*1000 + x.durationMinutes*60*1000)

		var ourTimezoneOffset = -new Date().getTimezoneOffset()
		var eventTimeZone = x.timeZone*60
		var timeZoneDifferentMinutes = ourTimezoneOffset - eventTimeZone
		startDate = new Date(startDate.getTime() + timeZoneDifferentMinutes*60*1000)
		endDate = new Date(endDate.getTime() + timeZoneDifferentMinutes*60*1000)

		var now = new Date();

		var eventOver = false

		if(now >= endDate) {
			eventOver = true
			$('eventOver').hidden = false
			$('eventInProgress').hidden = true
		}
		else if(now >= startDate) {
			$('eventOver').hidden = true
			$('eventInProgress').hidden = false
		}
		else {
			$('eventOver').hidden = true
			$('eventInProgress').hidden = true
		}

		let timeZoneString = 'GMT'
		if(x.timeZone >= 0) {
			timeZoneString += '+' + x.timeZone
		}
		else {
			timeZoneString += x.timeZone
		}

		$('eventDateTime').innerHTML = x.date + '<br><br>' + x.time + ' ' + timeZoneString
		$('eventDuration').innerHTML = durationToString2(x.durationHours, x.durationMinutes)

		$('bCalandar').onclick = _ => {
			var ics = 'BEGIN:VCALENDAR\nVERSION:2.0\nBEGIN:VEVENT\n'
			ics += 'SUMMARY:' + x.name.replaceAll('\n', '') + '\n'
			ics += 'DTSTART:'
			ics += x.date.replaceAll('-', '')
			ics += 'T'
			ics += x.time.replaceAll(':', '')
			ics += '00\nDTEND:'

			// 2021-01-15T20:31:18.680Z
			ics += endDate.toISOString().replaceAll('-', '').split('.')[0].replaceAll(':', '')

			ics += '\nEND:VEVENT\nEND:VCALENDAR\n\n'
			downloadFile('event.ics', ics, 'text/calendar')
		}

		if(x.youtubeVideoCode == null || x.youtubeVideoCode.length != 11) {
			$('youtube_vid').innerHTML = ''
		}
		else {
			$('youtube_vid').innerHTML = '<iframe width="280" height="160" src="https://www.youtube.com/embed/'+x.youtubeVideoCode+'" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>'
		}

		$('description').innerHTML = filterHTML(x.description, true, true)
		$('eventCountry').innerHTML = x.country
		$('eventAddress').innerHTML = filterHTML(x.address, true)

		if(x.gps == null) {
			$('eventGPS').innerHTML = ''
		}
		else {
			$('eventGPS').innerHTML = 'GPS: ' + x.gps[0] + ',' + x.gps[1]
		}

		$('addMedia').hidden = !weAreOrganiser
		if(x.media != null) {
			let html = ''
			x.media.forEach(m => {
				if(!m.isCoverImage) {
					html += getMediaPreviewHTML(m, weAreOrganiser)
				}
			})

			$('eventMedia').innerHTML = html

			x.media.forEach(m => {
				addMediaPreviewCallbacks(m, weAreOrganiser)
			})
		}
		else {
			$('eventMedia').innerHTML = ''
		}

		$('addMedia').onclick = _ => {
			loadAddMediaForm(args.id)
			fadeInElement(hideScreenVisibilityState)
			fadeInElement(addMediaVisibilityState)
		}

		$('eventExtendedDescription').innerHTML = filterHTML(x.description2, true, true)

		if(sessionStorage.myID == null || x.organiser_id == sessionStorage.myID) {
			$('bAttend').hidden = true
		}
		else {
			$('bAttend').hidden = false
			if(x.attending == true) {
				$('bAttend').innerHTML = 'Unregister Attendance'
				$('bAttend').disabled = eventOver

				$('bAttend').onclick = _ => {
					loadFile('unregister_attendance', _ => {
						$('bAttend').disabled = true
					}, _ => alert('Error'), 'POST', 'id=' + args.id)					
				}
			}
			else {
				$('bAttend').innerHTML = 'Register Attendance'
				$('bAttend').disabled = eventOver

				$('bAttend').onclick = _ => {
					loadFile('register_attendance', _ => {
						$('bAttend').disabled = true
					}, _ => alert('Error'), 'POST', 'id=' + args.id)	
				}
			}
		}

		let attendeesList = $('attendeesList')
		attendeesList.innerHTML = ''
		$('bShowAttendees').disabled = false
		$('bShowAttendees').hidden = x.attendeesListPrivate && x.organiser_id != sessionStorage.myID
		$('bShowAttendees').onclick = _ => {
			if(attendeesList.innerHTML == '') {
				loadJSON('attendees', x => {
					if(x.attendees != null) {
						let html = '<br>'
						x.attendees.forEach(attendee => {
							html += filterHTML(attendee.name) + '<br>'
						})
						attendeesList.innerHTML = html
					}
					$('bShowAttendees').disabled = true
				}, null, 'GET', 'id='+args.id)

			}
		}

		eventPageLoadComments(x, args.id, x.organiser_id)
		


		showActivePage()
	}, e => {
		if (e == 401) {
			alert('Not logged in')
		}
		else if (e == 403) {
			alert('You have not been invited to this event')
		}
		switchToPage('')
	}, 'GET', 'id=' + args.id)
}

function loadEditEventPage(args) {
	// args.id, is undefined for a new event
	// If args.id is defined then it is the ID of a draft or published event to modify

	if(args.id == null) {
		$('editEventPageTitle').innerHTML = 'New Event'
	} else {
		$('editEventPageTitle').innerHTML = 'Edit Event'
	}

	function publicStatusChanged() {
		let isPublic = $('ee_public').checked
		$('eePublicWarning').hidden = !isPublic
		$('ee_attendeesListPrivate').disabled = !isPublic
	}

	$('ee_public').onclick = _ => publicStatusChanged()


	let eeUseCurrentLocation = $('eeUseCurrentLocation')
	let geoLocAvailable = false
	if (navigator.geolocation) {
		geoLocAvailable = true
		eeUseCurrentLocation.onclick = _ => {
			navigator.geolocation.getCurrentPosition(p => {
				$('ee_gpsLat').value = p.coords.latitude
				$('ee_gpsLong').value = p.coords.longitude
			},function() {
				geoLocAvailable = false
			},{timeout:10000});
		}
	}

	function countryChanged() {
		eventEditPageSelectedCountry = $('ee_countryCode').value
		let d = eventEditPageSelectedCountry == ''
		$('ee_postCode').disabled = d
		$('ee_gpsLong').disabled = d
		$('ee_gpsLat').disabled = d
		$('ee_address').disabled = d
		$('eeUseCurrentLocation').disabled = d || !geoLocAvailable
	}
	countryChanged()
	$('ee_countryCode').onchange = countryChanged



	let all_data_fields = [
		'name',
		'description',
		'description2',
		'url',
		'public',
		'attendeesListPrivate',
		'date',
		'time',
		'timeZone',
		'durationHours',
		'durationMinutes',
		'countryCode',
		'address',
		'postCode',
		'gpsLat',
		'gpsLong',
		'youtubeVideoCode'
	]

	function setSubmitCallback(x) {
		let publishPressed = false

		$('editEventPublish').onclick = _ => {
			if(confirm('You are about to publish this event - it is not possible to reverse this action.\nAre you sure you want to publish this event? Press cancel to save the event without publishing.')) {
				publishPressed = true
			}
		}

		$('editEventForm').onsubmit = _ => {
			let postArgs = ''
			if(args.id != null) {
				postArgs = 'id=' + args.id
			}
			if(publishPressed) {
				postArgs += '&publish=1'
			}

			all_data_fields.forEach(field => {
				if($('ee_' + field).type == 'checkbox') {
					let checked = $('ee_' + field).checked
					if(checked != x[field]) {
						postArgs += '&' + field + '=' + (checked ? '1' : '0')
					}
				}
				else {
					let value = $('ee_' + field).value
					if(value == '') {
						value = null
					}
					if(value != x[field]

						// If date or time changes then send both since they are one field server-side
						|| (field == 'date' || field == 'time') &&
							($('ee_date').value != x['date'] || $('ee_time').value != x['time'])

						// Same as above
						|| (field == 'durationHours' || field == 'durationMinutes') &&
							($('ee_durationHours').value != x['durationHours'] || 
								$('ee_durationMinutes').value != x['durationMinutes'])

					) {

						if(field == 'time' && value.length > 5) {
							// Remove seconds component
							value = value.substr(0, 5)
						}

						if(value == null) {
							value = ''
						}
						postArgs += '&' + field + '=' + encodeURIComponent(value)
					}
					$('ee_' + field).value = value == null ? '' : value
				}
			})

			// invitations
			let email_divs = $('ee_emails').children
			if(email_divs.length > 0 || x.invitations != null) {
				if(x.invitations == null) {
					x.invitations = []
				}

				let removedEmails = []
				let newEmails = []
				for(let i = 0; i < email_divs.length; i++) {
					let email = email_divs[i].children[0].value
					if(email.trim().length == 0) {
						continue
					}

					// Was this email in the data before?
					let yesItWas = false

					for(let j = 0; j < x.invitations.length; j++) {
						if(x.invitations[j].trim() == email.trim()) {
							yesItWas = true
							break
						}
					}

					if(!yesItWas) {
						// We've added a new email then.
						newEmails.push(email)
					}
				}

				x.invitations.forEach(email => {
					// Is this email still in the list?
					let yesItIs = false
					for(let j = 0; j < email_divs.length; j++) {
						if(email_divs[j].children[0].value.trim() == email.trim()) {
							yesItIs = true
							break
						}
					}

					if(!yesItIs) {
						// We've delete an email then.
						removedEmails.push(email)
					}
				})

				if(removedEmails.length > 0) {
					postArgs += '&removeInvitee=' + removedEmails
				}
				if(newEmails.length > 0) {
					postArgs += '&addInvitee=' + newEmails
				}
			}

			loadFile('save_event', _ => {
				switchToPage('myEvents')
			}, _ => {
				alert('An error occurred')
			}, 'POST', postArgs)

			

			return false
		}
	}

	let email_field_last_id = 0

	function create_email_input_html(i, email) {
		if(email == null) {
			email = ''
		}
		return '<div id="ee_emailDiv' + i + '"><input id="ee_email' + i
					+ '" required value="' + email + '"> <button id="ee_removeEmail' + i + 
					'" type="button">Remove</button><br><br></div>'
	}

	if(args.id == null) {
		setSubmitCallback({})

		// Clear all fields

		all_data_fields.forEach(field => {
			if($('ee_' + field).type != 'checkbox') {
				$('ee_' + field).value = ''
			}
		})
		$('ee_emails').innerHTML = ''

		// Default values
		$('ee_public').checked = true
		$('ee_attendeesListPrivate').checked = false
		$('ee_timeZone').value = 0
		$('editEventPublish').disabled = false


		showActivePage()
	}
	else {
		// Load event info

		loadJSON('event_edit_info', x => {
			$('editEventPublish').disabled = x.isPublished
			eventEditPageSelectedCountry = x.countryCode

			all_data_fields.forEach(field => {
				let value = x[field]
				if(typeof(value) == 'boolean') {
					$('ee_' + field).checked = value
				}
				else {
					$('ee_' + field).value = value == null ? '' : value
				}
			})
			publicStatusChanged()

			if(!x.isPublished) {
				if(x.invitations == null) {
					$('ee_emails').innerHTML = ''
				}
				else {
					let html = ''
					for(let i = 0; i < x.invitations.length; i++) {
						let email = x.invitations[i]
						html += create_email_input_html(email_field_last_id, email)
						email_field_last_id += 1
					}
					$('ee_emails').innerHTML = html


					for(let i = 0; i < x.invitations.length; i++) {
						$('ee_removeEmail'+i).onclick = _ => $('ee_emailDiv'+i).remove()
					}
				}
			}
			

			setSubmitCallback(x)

			showActivePage()
		}, _ => {
			hideLoadingSymbol()
			alert('Error')
		}, 'GET', 'id='+args.id)
	}

	$('ee_addEmail').onclick = _ => {
		let i = email_field_last_id
		email_field_last_id += 1
		let child = document.createElement('div')
		child.innerHTML = create_email_input_html(i, null)
		$('ee_emails').appendChild(child.children[0])
		$('ee_removeEmail'+i).onclick = _ => $('ee_emailDiv'+i).remove()
	}



	// // 
	// window.onbeforeunload = function (e) {
	//     let m = 'You may have unsaved changes'

	//     e.returnValue = m;
	//     return m;
	// };
	
}

function loadMyEventsPage(args) {
	loadJSON('my_events', x => {
		createEventPreviews(x.events, $('my_events'), true)

		showActivePage()
	}, _ => switchToPage(''))
}

function loadUpcomingEventsPage(args) {
	loadJSON('upcoming_events', x => {
		createEventPreviews(x.events, $('upcoming_events'))

		showActivePage()
	}, _ => switchToPage(''))
}

function loadInvitationsPage(args) {
	loadJSON('events_invited_to', x => {
		createEventPreviews(x.events, $('events_invitited_to'))

		showActivePage()
	}, _ => switchToPage(''))
}



function loadUserPage(args) {
	let profilePicture = $('profilePicture')
	let profileName = $('profileName')
	let profileBIO = $('profileBIO')
	let profileNameEdit = $('profileNameEdit')
	let profileBIOEdit = $('profileBIOEdit')
	let profileBIOSave = $('profileBIOSave')
	let profileAddPic = $('profileAddPic')
	let profileRmPic = $('profileRmPic')
	let userModeratorAdminStatus = $('userModeratorAdminStatus')
	let toggleModStatus = $('toggleModStatus')
	let followButton = $('followButton')

	userModeratorAdminStatus.innerHTML = ''
	toggleModStatus.style.display = 'none'
	followButton.style.display = 'none'

	$('myProfileElements').style.display = args.id != sessionStorage.myID ? 'none' : 'inline'
	$('notMyProfileElements').style.display = args.id == sessionStorage.myID ? 'none' : 'inline'

	profileRmPic.style.display = sessionStorage.myProfPicID == null ? 'none' : 'inline'


	loadJSON('user_profile', x => {
		if(x.privilegeLevel == 1) {
			userModeratorAdminStatus.innerHTML = 'Moderator'
		}
		else if(x.privilegeLevel == 2) {
			userModeratorAdminStatus.innerHTML = 'Admin'
		}

		if(sessionStorage.myID != null && args.id != sessionStorage.myID) {
			followButton.style.display = 'inline'
			if(x.following) {
				followButton.innerHTML = 'Unfollow'
			}
			else {
				followButton.innerHTML = 'Follow'
			}

			followButton.onclick = _ => {
				if(x.following) {
					loadFile('unfollow', _ => {
						followButton.innerHTML = 'Follow'
						x.following = false
					}, _ => alert('Error'), 'POST', 'id=' + args.id)
				} else {
					loadFile('follow', _ => {
						followButton.innerHTML = 'Unfollow'
						x.following = true
					}, _ => alert('Error'), 'POST', 'id=' + args.id)
				}
			}
		}

		if(sessionStorage.myPrivilegeLevel >= 2 && x.privilegeLevel <= 1 && args.id != sessionStorage.myID) {
			let modButtonText = ['Grant Moderator Status', 'Revoke Moderator Status']
			toggleModStatus.innerHTML = modButtonText[x.privilegeLevel]
			toggleModStatus.style.display = 'inline'

			let new_priv = (x.privilegeLevel + 1) % 2

			toggleModStatus.onclick = _ => {
				loadFile('set_user_priv', _ => {
					if(new_priv == 0){
						userModeratorAdminStatus.innerHTML = ''
					}
					else {
						userModeratorAdminStatus.innerHTML = 'Moderator'
					}
					toggleModStatus.innerHTML = modButtonText[new_priv]
					new_priv = (new_priv + 1) % 2
				}, _ => alert('Error'), 'POST', 'id=' + args.id + '&priv=' + new_priv)
			}

		}

		if(args.id != sessionStorage.myID) {
			profileName.innerHTML = filterHTML(x.name)
			profileBIO.innerHTML = filterHTML(x.bio, true, true)
		}
		else {
			profileNameEdit.value = x.name
			profileBIOEdit.value = x.bio

			profileBIOSave.onclick = _ => {
				if(profileNameEdit.value != x.name || profileBIOEdit.value != x.bio) {
					loadFile('save_profile', _ => {
						alert('Success')
						if(profileNameEdit.value != x.name) {
							window.location.href = window.location.href
						}
					}, _ => alert('Error'), 'POST', 
						'name=' + encodeURIComponent(profileNameEdit.value) + 
						'&bio=' + encodeURIComponent(profileBIOEdit.value))
				}
			}
		}

		profileRmPic.style.display = (args.id != sessionStorage.myID || !x.profPic) ? 'none' : 'inline'

		if(x.profPic != null) {
			profilePicture.src = S3_URL + x.profPic
		}
		else {
			profilePicture.src ='ico/blank-prof-pic.svg'
		}

		profileRmPic.onclick = _ => {
			loadFile('remove_profile_picture', _ => {
				profilePicture.src ='ico/blank-prof-pic.svg'
				window.location.href = window.location.href
			}, _ => alert('error'), 'POST')
		}

		profileAddPic.onclick = _ => {
			fadeInElement(hideScreenVisibilityState)
			fadeInElement(addProfilePictureVisibilityState)
			loadAddProfilePictureForm()		
		}

		showActivePage()
	}, _ => switchToPage(''), 'GET', 'id=' + args.id)
}

function loadPasswordResetPage(args) {
	if(args['code'] == null) {
		alert('Invalid URL - missing code')
		switchToPage('')
		return
	}
	if(args['code'].length != 32 || !args['code'].match(/[0-9A-Fa-f]/g)) {
		alert('Invalid link - check you copy-pasted the URL correctly')
		switchToPage('')
		return
	}

	// Verify code is valid and get email

	loadJSON('verify_password_reset_link', x => {
		$('pwdResetEmail1').innerHTML = x.email
		$('pwdResetExpiresIn').innerHTML = x.expiresIn

		$('pwdResetShowAfterConfirmCode').hidden = false

		$('pwdResetForm').onsubmit = _ => {
			loadFile('password_reset', _ => {
				alert('Password changed successfully.')
				logIn(x.email, $('pwdResetPassword').value, true)
			}, _ => {
				alert('An error occurred')
				switchToPage('')
			}, 
			'POST',
			'code=' + args['code']
				+ '&pwd=' + encodeURIComponent($('pwdResetPassword').value))

			return false
		}
	}, e => {
		if(Math.floor(e/100) == 4) {
			alert('The password reset link you used was invalid. Be aware that links expire after 10 minutes.')
		}
		else {
			alert('A sever-side error occurred. Please try again another time.')
			switchToPage('')
		}
	}, 'GET', 'code=' + args['code'])

	showActivePage()
}


function setUserInfo(id, name, profPicID, email, privilegeLevel) {
	sessionStorage.myName = name
	sessionStorage.myID = id
	sessionStorage.myEmail = email
	sessionStorage.myPrivilegeLevel = privilegeLevel


	if(profPicID != null) {
		sessionStorage.myProfPicID = profPicID
	}
	else {
		sessionStorage.myProfPicID = null
	}

	$('navbar_name').innerHTML = name
	$('usersEmail').innerHTML = filterHTML(email)
	$('usersEmail').onclick = _ => {
		fadeOutElement(userDropdownVisibilityState)
		switchToPage('user', {id: id})
	}

	$('navbar_prof_pic').hidden = false
	if(profPicID != null) {
		$('navbar_prof_pic').src = S3_URL + profPicID
	}
	else {
		$('navbar_prof_pic').src = "ico/blank-prof-pic.svg"
	}
}

// Called when website is opened
function loadProfPicAndName(cb) {
	loadJSON('my_basic_info', info => {
		setUserInfo(info.id, info.name, info.profPic, info.email, info.privilegeLevel)
		cb()
	}, function() {
		// Not logged in
		$('navbar_name').innerHTML = 'Sign In'
		$('navbar_prof_pic').hidden = true

		sessionStorage.removeItem('myID')
		sessionStorage.removeItem('myName')
		sessionStorage.removeItem('myProfPicID')
		sessionStorage.removeItem('myEmail')

		cb()
	})

}

function logIn(email, pwd, goHome=false) {
	let formData = 'email=' + encodeURIComponent(email.trim())
		+ '&pwd=' + encodeURIComponent(pwd)

	loadJSON('login', x => {
		fadeOutElement(logInFormVisibilityState)

		if(goHome) {
			// Reload page
			window.location.href = window.location.href.split('?')[0]
		}
		else {
			window.location.href = window.location.href
		}
	}, _ => {
		alert('An error occurred')
	}, 'POST', formData)
}

function loadNavbarName() {
	$('navbar_prof_pic_and_name').onclick = e => {
		if(logInFormVisibilityState.visible != UIVisibilityState.invisible ||
			userDropdownVisibilityState.visible != UIVisibilityState.invisible ||
			pwdResetFormVisibilityState.visible != UIVisibilityState.invisible ||
			signUpFormVisibilityState.visible != UIVisibilityState.invisible) {
			// A dropdown is active. Hide it.
			fadeOutElement(logInFormVisibilityState)
			fadeOutElement(userDropdownVisibilityState)
			fadeOutElement(pwdResetFormVisibilityState)
			fadeOutElement(signUpFormVisibilityState)
		}
		else {
			if(sessionStorage.myName == null) {
				fadeInElement(logInFormVisibilityState)
			}
			else {
				fadeInElement(userDropdownVisibilityState)
			}
		}
	}
}

function loadLoginForm() {
	

	$('loginForm').onsubmit = e => {
		logIn($('loginEmail').value, $('loginPassword').value)

		return false
	}

	let logInEmail = $('loginEmail')
	logInEmail.onchange = _ => emailValidation(logInEmail)

	$('switchToSignupForm').onclick = e => {
		fadeOutElement(logInFormVisibilityState, _ => fadeInElement(signUpFormVisibilityState))
	}
	$('switchToPwdResetForm').onclick = e => {
		fadeOutElement(logInFormVisibilityState, _ => fadeInElement(pwdResetFormVisibilityState))
	}
}


function passwordValidation(pwd, pwdConfirm) {
	if(pwd.value == pwdConfirm.value) {
		pwdConfirm.setCustomValidity('');
	} else {
		pwdConfirm.setCustomValidity("Passwords must match");
	}

	if(pwd.value.length >= 8) {
		pwd.setCustomValidity('');
	} else {
		pwd.setCustomValidity("Passwords must be at least 8 characters long");
	}
}

function emailValidation(emailInput) {
	let e = emailInput.value

    let INV_EMAIL = 'Invalid email address'

	if(e.length < 5 || e[0] == '@' || e[0] == '.' || e[e.length-1] == '@' || e[e.length-1] == '.') {
		emailInput.setCustomValidity(INV_EMAIL)
		return
	}


	let atSymbolFound = false
    let dotSymbolFoundAfterAt = false


    for(let i = 0; i < e.length; i++) {
    	if(e[i] == '@') {
    		if(atSymbolFound) {
    			emailInput.setCustomValidity(INV_EMAIL);
    			break
    		}
    		atSymbolFound = true
    	}
    	if(e[i] == '.') {
    		if(atSymbolFound) {
    			dotSymbolFoundAfterAt = true;
    		}
    	}
    }
    if(atSymbolFound && dotSymbolFoundAfterAt) {
    	emailInput.setCustomValidity('')
    }
    else {
    	emailInput.setCustomValidity(INV_EMAIL);
    }
}

function loadPasswordResetForm2() {
	let pwd = $('pwdResetPassword')
	let pwdConfirm = $('pwdResetConfirmPassword')
	pwd.onchange = _ => passwordValidation(pwd, pwdConfirm)
	pwdConfirm.onchange = _ => passwordValidation(pwd, pwdConfirm)
}

function loadSignupForm() {
	let signUpPassword = $('signUpPassword')
	let signUpConfirmPassword = $('signUpConfirmPassword')
	let signUpEmail = $('signUpEmail')

	signUpPassword.onchange = _ => passwordValidation(signUpPassword, signUpConfirmPassword)
	signUpConfirmPassword.onchange = _ => passwordValidation(signUpPassword, signUpConfirmPassword)
	signUpEmail.onchange = _ => emailValidation(signUpEmail)


	$('signUpForm').onsubmit = e => {
		let formData = 'email=' + encodeURIComponent(signUpEmail.value.trim())
			+ '&pwd=' + encodeURIComponent(signUpPassword.value)
			+ '&name=' + encodeURIComponent($('signUpName').value.trim())

		loadJSON('signup', x => {
			fadeOutElement(signUpFormVisibilityState)

			// Reload page
			window.location.href = window.location.href
		}, _ => {
			alert('An error occurred')
		}, 'POST', formData)

		return false
	}

	
	$('switchToLoginForm').onclick = e => {
		fadeOutElement(signUpFormVisibilityState, _ => fadeInElement(logInFormVisibilityState))
	}
}

function logOut() {
	sessionStorage.removeItem('myID')
	sessionStorage.removeItem('myName')
	sessionStorage.removeItem('myProfPicID')
	sessionStorage.removeItem('myPrivilegeLevel')

	// Reload page
	window.location.href = window.location.href
}

function loadUserDropdown() {
	$('logOut').onclick = e => {
		loadFile('logout', s => {
			logOut()
		}, _ => {
			alert('An error occurred')
		}, 'POST')
	}
}

function loadPasswordResetForm() {
	let email = $('pwdResetEmail')
	email.onchange = _ => emailValidation(email)

	$('passwordResetForm').onsubmit = e => {
		let formData = 'email=' + encodeURIComponent($('pwdResetEmail').value.trim())

		loadFile('get_password_reset_link', _ => {
			alert('Check your email inbox')
			fadeOutElement(pwdResetFormVisibilityState)
		}, _ => {
			alert('An error occurred')
		}, 'POST', formData)

		return false
	}
}

function unloadPage() {
	window.onbeforeunload = null

	// Clear dynamic content
	let elements = document.getElementsByClassName('page_dynamic_content')
	for(let i = 0; i < elements.length; i++) {
		elements[i].innerHTML = ''
	}

	
	hideActivePage()
	active_page = null
}

function loadPage(pageName, args={}) {
	active_page = pageName

	Pages[pageName](args)
}

function switchToPage(pageName, args={}) {
	let queryString = '/'
	if(pageName != '') {
		queryString = '/?page=' + pageName
		for(let arg in args) {
			queryString += '&' + arg + '=' + args[arg]
		}
	}
	window.history.pushState({} , '', queryString);


	unloadPage()
	loadPage(pageName, args)
}

function loadInitialPage() {
	// Find which page to load by searching for the page=... in the URL
	// (defaults to home page)

	let pageToLoad = null
	let args = {}

	if(window.location.search) {
		let s = window.location.search.substr(1).split("&")
		for(let i = 0; i < s.length; i++) {
			if(s[i].startsWith("page=")) {
				if(pageToLoad == null) {
					pageToLoad = s[i].substr(5)
				}
			}
			else {
				let x = s[i].split('=')
				args[x[0]] = x[1]
			}
		}
	}

	if(pageToLoad == null) {
		pageToLoad = ''
	}
	else {
		// Check page is valid

		let pageIsValid = false
		let validPages = Object.keys(Pages)
		for(let i = 0; i < validPages.length; i++) {
			if(validPages[i] == pageToLoad) {
				pageIsValid = true
				break;
			}
		}
		if(!pageIsValid) {
			pageToLoad = ''
		}
	}

	loadPage(pageToLoad, args)
}

function onNotificationsPanelClosed() {
	if(notificationsDropdownVisibilityState.visible == UIVisibilityState.visible) {
		lastSeenNotificationID = lastNotificationID
		regenerateNotificationsHTML()
	}
}

function loadNavMenuCallbacks() {
	$('hamburgerIcon').onclick = _ => {
		fadeInOrOutElement(lhsDropdownVisibilityState)
	}

	$('bellIconContainer').onclick = _ => {
		lastSeenNotificationID = lastNotificationID
		updateNotificationsSeen()
		$('notificationsNumber').hidden = true
		onNotificationsPanelClosed() // function checks if we are actually closing the panel
		fadeInOrOutElement(notificationsDropdownVisibilityState)
	}

	$('nav_home').onclick = _ => {
		fadeOutElement(lhsDropdownVisibilityState)
		switchToPage('')
	}
	$('nav_create_event').onclick = _ => {
		fadeOutElement(lhsDropdownVisibilityState)
		if(sessionStorage.myID == null) {
			alert('Not logged in')
		} else {
			switchToPage('editEvent')
		}
	}
	$('nav_my_events').onclick = _ => {
		fadeOutElement(lhsDropdownVisibilityState)
		if(sessionStorage.myID == null) {
			alert('Not logged in')
		} else {
			switchToPage('myEvents')
		}
	}

	$('nav_upcoming_events').onclick = _ => {
		fadeOutElement(lhsDropdownVisibilityState)
		if(sessionStorage.myID == null) {
			alert('Not logged in')
		} else {
			switchToPage('upcomingEvents')
		}
	}

	$('nav_invitations').onclick = _ => {
		fadeOutElement(lhsDropdownVisibilityState)
		if(sessionStorage.myID == null) {
			alert('Not logged in')
		} else {
			switchToPage('invitations')
		}
	}
}

function lazyLoadImages() {
	let images = document.getElementsByTagName("img")
	for (let i = 0; i < images.length; i++) {
		let img = images[i]
		let src = img.getAttribute('data-lazy-load-src')
		if(src != null) {
			img.src = src
		}
	}
}

// Sorted by ID descending
let allNotifications = []
let lastNotificationID = 0
let lastSeenNotificationID = 0
let unseenNotifications = 0
let notificationsRefreshInterval = null

function regenerateNotificationsHTML() {
	if(sessionStorage.myID == null) {
		$('notificationsDropdown').innerHTML = 'You are not logged in.<br>Log in or sign up to get notifications for events you are participating in and users you are following.'
		return
	}

	if(allNotifications.length == 0) {
		$('notificationsDropdown').innerHTML = 'No notifications'
		return
	}

	let html = ''
	allNotifications.forEach(n => {
		let unseen = false
		if(n.id > lastSeenNotificationID) {
			unseenNotifications += 1
			unseen = true
		}


		html += '<div id="notif_' + n.id + '" class="notification'
		if(n.eventID != null) {
			html += " clickable"
		}
		if(unseen) {
			html += " unseenNotification"
		}
		html += '"">'

		if(n.type == 'EventChanged') {
			html += 'The event <i>' + filterHTML(n.eventName) + '</i> has been modified.'
		}
		else if (n.type == 'NewEvent') {
			html += filterHTML(n.organiserName) + ' has created the event <i>' + filterHTML(n.eventName) + '</i>.';
		}
		else if (n.type == 'EventDeleted') {
			html += 'The event <i>' + filterHTML(n.eventName) + '</i> has been cancelled.'
		}

		html += '<br><span class="notificationTimeSince">' + secondsToTimeString(n.timeSince) + ' ago</span>'


		html += '</div><br>'
		if(n.id > lastNotificationID) {
			lastNotificationID = n.id
		}
	})
	$('notificationsDropdown').innerHTML = html

	// Set onclick callbacks
	allNotifications.forEach(n => {
		if(n.eventID != null) {
			$('notif_' + n.id).onclick = _ => switchToPage('event', {id: n.eventID})
		}
	})
}

function getNotifications() {
	if(sessionStorage.myID == null) {
		regenerateNotificationsHTML()
		return
	}

	let args = 'from_id=0'
	if(lastNotificationID > 0) {
		args = 'from_id=' + (lastNotificationID+1)
	}

	loadJSON('notifications', x => {
		if(x.prevLastSeenID > lastSeenNotificationID) {
			lastSeenNotificationID = x.prevLastSeenID
		}
		if(x.notifications != null && x.notifications.length > 0) {
			allNotifications = allNotifications.concat(x.notifications.slice().reverse())

		}
		regenerateNotificationsHTML()


		if(notificationsDropdownVisibilityState.visible == UIVisibilityState.visible) {
			// User has notifications window open. They have seen the new notifications (if there were any)
			lastSeenNotificationID = lastNotificationID
			updateNotificationsSeen()
		}

		let notificationsNumber = $('notificationsNumber')
		if(unseenNotifications == 0){
			notificationsNumber.hidden = true
		}
		else {
			notificationsNumber.innerHTML = unseenNotifications
			notificationsNumber.hidden = false
		}
	}, null, 'GET', args)

	if(notificationsRefreshInterval == null) {
		notificationsRefreshInterval = setInterval(getNotifications, 5*60*1000)
	}
}

function updateNotificationsSeen() {
	loadFile('mark_notifications_seen', _ => {}, null, 'POST', 'latest=' + lastSeenNotificationID)
}

// let countries = ['Online', 'United Kingdom']
let eventEditPageSelectedCountry = null

function getCountriesList() {
	loadStaticTextFile('countries.txt', x => {
		let select = document.createElement("select");
		select.id = 'ee_countryCode'

		let i = 0
		x.split('\n').forEach(c => {
			let t = c.trim()
			if(t.length > 0) {
				// countries.push(t)

				let option = document.createElement("option");
			    option.value = ''+i;
			    i += 1
			    option.text = t
			    select.appendChild(option);
			}
		})


		let ee_countryCode_container = $('ee_countryCode_container')
		ee_countryCode_container.innerHTML = ''
		ee_countryCode_container.appendChild(select)

		if(eventEditPageSelectedCountry != null) {
			$('ee_countryCode').value = eventEditPageSelectedCountry
		}
	})
}


function loadAddCoverImageForm(eventID) {
	$('addCoverImageFile').value = ''
	$('uploadCoverImageForm').onsubmit = _ => {
		var file = $('addCoverImageFile')

		function closeForm() {
			fadeOutElement(hideScreenVisibilityState)
			fadeOutElement(addCoverImageVisibilityState)
		}

		uploadFile(file, 'add_cover_image', '&event_id='+eventID, x => {
			eventPageHasCoverImage(x.id, true, eventID)
			closeForm()
		}, e => {
			alert(e)
			closeForm()
		})

		return false
	}
}

function loadAddMediaForm(eventID) {
	$('addMediaFile').value = ''
	$('uploadMediaForm').onsubmit = _ => {
		try{
		var file = $('addMediaFile')

		function closeForm() {
			fadeOutElement(hideScreenVisibilityState)
			fadeOutElement(addMediaVisibilityState)
		}

		uploadFile(file, 'add_media', '&event_id='+eventID, x => {
			let div = document.createElement("div");
			div.innerHTML = getMediaPreviewHTML(x, true)
			$('eventMedia').appendChild(div);
			addMediaPreviewCallbacks(x, true)
			closeForm()
		}, e => {
			alert(e)
			closeForm()
		})

	}
	catch(e) {
		console.error(e)
	}
		return false
	}
}

function loadAddProfilePictureForm(eventID) {
	$('addProfilePictureFile').value = ''
	$('uploadProfilePictureForm').onsubmit = _ => {
		var file = $('addProfilePictureFile')

		function closeForm() {
			fadeOutElement(hideScreenVisibilityState)
			fadeOutElement(addProfilePictureVisibilityState)
		}

		uploadFile(file, 'new_profile_picture', null, x => {
			$('profilePicture').src = S3_URL + x.id
			$('navbar_prof_pic').src = S3_URL + x.id
			sessionStorage.myProfPicID = x.id
			closeForm()
			window.location.href = window.location.href
		}, e => {
			alert(e)
			closeForm()
		})

		return false
	}
}


window.onload = function() {
	var new_script = document.createElement('script')
	new_script.type = 'text/javascript'
	new_script.src = 'lib/velocity1.5.2.min.js'
	var first_script = document.getElementsByTagName('script')[0]
	first_script.parentNode.insertBefore(new_script, first_script)

	if ('serviceWorker' in navigator) {
		navigator.serviceWorker.register('/sw.js').then(function(registration) {
			// Registration was successful
			console.log('ServiceWorker registration successful with scope: ', registration.scope);
		}, function(err) {
			// registration failed :(
			console.error('ServiceWorker registration failed: ', err);
		});
	}
	
	loadProfPicAndName(_ => {
		getNotifications()
	})
	loadNavbarName()
	loadLoginForm()
	loadUserDropdown()
	loadSignupForm()
	loadPasswordResetForm()
	loadAddCoverImageForm()
	loadInitialPage()
	loadPasswordResetForm2()
	loadNavMenuCallbacks()
	lazyLoadImages()
	getCountriesList()

	window.onclick = function(event) {
		if(logInFormVisibilityState.visible == UIVisibilityState.visible) {
			if(!isOrIsChildOf(logInFormVisibilityState.element, event.target) && !isOrIsChildOf($('navbar_prof_pic_and_name'), event.target)) {
				fadeOutElement(logInFormVisibilityState)
			}
		}
		if(signUpFormVisibilityState.visible == UIVisibilityState.visible) {
			if(!isOrIsChildOf(signUpFormVisibilityState.element, event.target) && !isOrIsChildOf($('navbar_prof_pic_and_name'), event.target)) {
				fadeOutElement(signUpFormVisibilityState)
			}
		}
		if(pwdResetFormVisibilityState.visible == UIVisibilityState.visible) {
			if(!isOrIsChildOf(pwdResetFormVisibilityState.element, event.target) && !isOrIsChildOf($('navbar_prof_pic_and_name'), event.target)) {
				fadeOutElement(pwdResetFormVisibilityState)
			}
		}
		if(userDropdownVisibilityState.visible == UIVisibilityState.visible) {
			if(!isOrIsChildOf(userDropdownVisibilityState.element, event.target) && !isOrIsChildOf($('navbar_prof_pic_and_name'), event.target)) {
				fadeOutElement(userDropdownVisibilityState)
			}
		}
		if(lhsDropdownVisibilityState.visible == UIVisibilityState.visible) {
			if(!isOrIsChildOf(lhsDropdownVisibilityState.element, event.target) && !isOrIsChildOf($('hamburgerIcon'), event.target)) {
				fadeOutElement(lhsDropdownVisibilityState)
			}
		}
		if(notificationsDropdownVisibilityState.visible == UIVisibilityState.visible) {
			if(!isOrIsChildOf(notificationsDropdownVisibilityState.element, event.target) && !isOrIsChildOf($('bellIconContainer'), event.target)) {
				fadeOutElement(notificationsDropdownVisibilityState)
				onNotificationsPanelClosed()
			}
		}
		if(addCoverImageVisibilityState.visible == UIVisibilityState.visible) {
			if(!isOrIsChildOf(addCoverImageVisibilityState.element, event.target)) {
				fadeOutElement(addCoverImageVisibilityState)
				fadeOutElement(hideScreenVisibilityState)
			}
		}
		if(addMediaVisibilityState.visible == UIVisibilityState.visible) {
			if(!isOrIsChildOf(addMediaVisibilityState.element, event.target)) {
				fadeOutElement(addMediaVisibilityState)
				fadeOutElement(hideScreenVisibilityState)
			}
		}
		if(addProfilePictureVisibilityState.visible == UIVisibilityState.visible) {
			if(!isOrIsChildOf(addProfilePictureVisibilityState.element, event.target)) {
				fadeOutElement(addProfilePictureVisibilityState)
				fadeOutElement(hideScreenVisibilityState)
			}
		}

	}

	window.addEventListener('popstate', _ => {
		unloadPage()
		loadInitialPage()
	});
	
}