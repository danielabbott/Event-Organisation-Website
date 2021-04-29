"use strict";

// Reply if c.numberOfReplies == null
function makeCommentOrReplyHtml(c, eventOrganiserID) {
	let commentID = ''
	if(c.numberOfReplies == null) {
		commentID = 'reply'+c.id
	}
	else {
		commentID = 'comment'+c.id
	}

	let html = '<div class="comment'

	if(c.userID == eventOrganiserID) {
		html += ' organiserComment'
	}

	html += '" id="'+commentID+'">' + '<img src="'

	if(c.commenterProfPic != null && c.commenterProfPic != 'undefined' && c.commenterProfPic != 'null') {
		html += S3_URL+c.commenterProfPic
	}
	else {
		html += 'ico/blank-prof-pic.svg'
	}

	html += '" class="commentProfPic">'
		+ '<div class="comment_rhs"><span class="commentName" id="name_' + commentID
		+ '"><b>' + filterHTML(c.commentersName) + '</b></span>'
		+ '&emsp;'+ secondsToTimeString(c.ago) + ' ago<br>'
		+ '<p class="commentText">' + filterHTML(c.text, true, true) + '</p>'

	if(c.userID == sessionStorage.myID || sessionStorage.myPrivilegeLevel > 0){
		let deleteID = ''
		let editID = ''
		if(c.numberOfReplies == null) {
			deleteID = 'replyDelete'+c.id
			editID = 'replyEdit'+c.id
		}
		else {
			deleteID = 'commentDelete'+c.id
			editID = 'commentEdit'+c.id
		}

		html += '<span class="commentButton" id="'+deleteID+'">Delete</span>&emsp;'
		
	}

	if(c.numberOfReplies != null) {
		if(c.numberOfReplies > 0) {
			html += '<span id="commentReplies' + c.id + '"></span><span class="commentButton" id="commentShowReplies' + c.id + '">'
			if(c.numberOfReplies == 1) {
				html += 'View 1 reply'
			}
			else {
				html += 'View ' + c.numberOfReplies + ' replies'
			}
			html += '</span>'
		}
		else {
			html += '<span id="commentReplies' + c.id + '"></span><span class="commentButton" id="commentReply' + c.id + '">Reply</span>'
		}
	}

	html += '</div></div><br>'
	return html
}
function getReplyTextBoxHTML(id) {
	if(sessionStorage.myID == null) {
		return '<i>Log in to join the conversation</i>';
	}
	return '<br><textarea class="commentTextArea" id="addReplyTextArea' + id + '"></textarea><br>'
			+ '<button class="greyButton" id="addReply' + id + '">Add reply</button>'
}

function setReplyCallbacks(r) {

	$('name_reply'+r.id).onclick = _ => {
		switchToPage('user', {id: r.userID})
	}

	if(r.userID == sessionStorage.myID || sessionStorage.myPrivilegeLevel > 0) {
		let del = $('replyDelete' + r.id)
		del.onclick = _ => {
			if(confirm("Delete reply?")) {
				let oldOnclick = del.onclick
				del.onclick = null
				loadFile("delete_reply", _ => {
					$('reply'+r.id).remove()
				}, _ => del.onclick = oldOnclick, 'POST', 'id=' + r.id)
			}
		}
	}
}

// Button for adding a reply
// Takes comment id
function setReplyListeners(id, eventOrganiserID) {
	if(sessionStorage.myID == null) {
		return;
	}

	$('addReply' + id).onclick = _ => {
		let addReplyTextArea = $('addReplyTextArea' + id)
		let text = addReplyTextArea.value.trim()
		addReplyTextArea.value = ''

		if(text.length > 0) {
			loadFile('post_reply', reply_id => {
				let r = {
					id: reply_id,
					userID: sessionStorage.myID,
					commenterProfPic: sessionStorage.myProfPicID,
					commentersName: filterHTML(sessionStorage.myName),
					text: text,
					ago: 0
				}
				let html = makeCommentOrReplyHtml(r, eventOrganiserID)

				let div = document.createElement('div');
				div.innerHTML = html
				$('commentReplies' + id).appendChild(div)
				setReplyCallbacks(r)
			}, 
			_ => alert('Error posting reply'), 'POST',
				'comment_id=' + id + '&text=' + encodeURIComponent(text))
		}
	}
}

// Callbacks for edit/delete/reply
function setCommentCallbacks(c, eventOrganiserID) {
	if(c.numberOfReplies < 1) {
		// Just show reply box
		
		let span = $('commentReply' + c.id)
		span.onclick = _ => {
			span.onclick = null
			span.classList.remove("commentButton");

			let html = '<br>' + getReplyTextBoxHTML(c.id)
			span.innerHTML = html
			setReplyListeners(c.id, eventOrganiserID)
		}
	}
	else {
		let span = $('commentShowReplies' + c.id)
		span.onclick = _ => {
			loadJSON('replies', x => {
				span.onclick = null
				span.classList.remove("commentButton");

				let html = ''
				if(x.replies != null) {
					x.replies.forEach(r => {
						html += makeCommentOrReplyHtml(r, eventOrganiserID)
					})
				}
				$('commentReplies'+c.id).innerHTML = html

				if(x.replies != null) {
					x.replies.forEach(r => setReplyCallbacks(r))
				}

				span.innerHTML = getReplyTextBoxHTML(c.id)
				setReplyListeners(c.id, eventOrganiserID)

			}, null, 'GET', 'id='+c.id)
		}
	}

	if(c.userID == sessionStorage.myID || sessionStorage.myPrivilegeLevel > 0) {
		let del = $('commentDelete' + c.id)
		del.onclick = _ => {
			if(confirm("Delete comment?")) {
				let oldOnclick = del.onclick
				del.onclick = null
				loadFile("delete_comment", _ => {
					$('comment'+c.id).remove()
				}, _ => del.onclick = oldOnclick, 'POST', 'id=' + c.id)
			}
		}
	}

	$('name_comment'+c.id).onclick = _ => {
		switchToPage('user', {id: c.userID})
	}
	
}

// x is the json object returned from the web server
function eventPageLoadComments(x, eventID, eventOrganiserID) {
	let lowestLoadedCommentID = 4294967295


	if(x.comments != null) {
		let html = ''

		x.comments.forEach(c => {
			if(c.id < lowestLoadedCommentID) {
				lowestLoadedCommentID = c.id
			}

			html += makeCommentOrReplyHtml(c, eventOrganiserID)
		})

		$('comments').innerHTML = html
		x.comments.forEach(c => setCommentCallbacks(c, eventOrganiserID))

		$('loadMoreComments').disabled = false
		$('loadMoreComments').onclick = _ => {
			loadJSON('comments', x2 => {
				if(x2.comments != null && x2.comments.length > 0) {
					let html = ''
					x2.comments.forEach(c => {
						if(c.id < lowestLoadedCommentID) {
							lowestLoadedCommentID = c.id
						}
						html += makeCommentOrReplyHtml(c, eventOrganiserID)
					})
					let div = document.createElement('div');
					div.innerHTML = html
					$('comments').appendChild(div)


					x2.comments.forEach(c => setCommentCallbacks(c, eventOrganiserID))
				}
				else {
					$('loadMoreComments').disabled = true
				}
			}, null, 'GET', 'eventid=' + eventID + '&startid=' + (lowestLoadedCommentID-1))
		}
	}
	else {
		$('loadMoreComments').disabled = true
		$('comments').innerHTML = ''
	}

	$('addComment').onclick = _ => {
		let addCommentTextArea = $('addCommentTextArea')
		let text = addCommentTextArea.value.trim()
		addCommentTextArea.value = ''

		if(text.length > 0) {
			loadFile('post_comment', id => {
				let c = {
					id: id,
					userID: sessionStorage.myID,
					commentersName: filterHTML(sessionStorage.myName),
					commenterProfPic: sessionStorage.myProfPicID,
					text: text,
					ago: 0,
					numberOfReplies: 0
				}
				let html = makeCommentOrReplyHtml(c, eventOrganiserID)

				let comments_html_obj = $('comments')
				if(comments_html_obj.childNodes.length > 0) {
					let div = document.createElement('div');
					div.innerHTML = html
					comments_html_obj.insertBefore(div, $('comments').childNodes[0])
				}
				else {
					comments_html_obj.innerHTML = html
				}

				setCommentCallbacks(c, eventOrganiserID)

				// Scroll to top of comment section

				if (typeof window.Velocity != 'undefined') {
					Velocity($('commentsSectionStart'), "scroll", { duration: 500, easing: "ease", offset: "-70px" })
				}
			}, 
			_ => alert('Error posting comment'), 'POST',
				'event_id=' + eventID + '&text=' + encodeURIComponent(text))
		}
	}

	if(sessionStorage.myID == null) {
		$('addCommentSectionEnabled').hidden = true

		if(x.comments == null || x.comments.length == 0) {
			$('addCommentSectionDisabled').hidden = true
			$('addCommentSectionDisabledNoComments').hidden = false
		}
		else {
			$('addCommentSectionDisabled').hidden = false
			$('addCommentSectionDisabledNoComments').hidden = true
		}
	}
	else {
		$('addCommentSectionEnabled').hidden = false
		$('addCommentSectionDisabled').hidden = true
		$('addCommentSectionDisabledNoComments').hidden = true
	}
}