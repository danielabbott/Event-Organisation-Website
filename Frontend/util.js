"use strict";
let $ = function(id) { return document.getElementById(id) }

function deleteHTML(e) {
	e.parentNode.removeChild(e);
}

// Asynchronous
// parameters can be either a string (query for GET, form data for POST/PUT/)
//	or can be a FormData for POST/PUT
function loadFile(file, callback, error_callback=null, method='GET', parameters=null) {
	if(method == 'GET') {
		if(parameters != null && typeof parameters == 'string') {
			file = file + '?' + parameters
			parameters = null
		}
		else {
			parameters = null
		}
	}

	let path = '/d/' + file
	let xhttp = new XMLHttpRequest();

	xhttp.onreadystatechange = function() {
	    if (this.readyState == 4) {
	        if(this.status == 200) {
	            callback(this.responseText)
	        }
	        else {
	            if(error_callback != null) {
	            	error_callback(this.status)
	            }
	            else {
	            	console.error('Error loading ' + path + ': ' + this.status);
	            }
	        }
	    }
	};
	xhttp.open(method, path, true);

	if(parameters != null && typeof parameters == 'string') {
		xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
	}

	if(parameters == null) {
		xhttp.send();
	}
	else {
		xhttp.send(parameters)
	}
}


// Asynchronous
function loadJSON(file, callback, error_callback=null, method='GET', parameters=null) {
	loadFile(file + '.json', responseText => {

		let j = null
		try {
			j = JSON.parse(responseText)
			if(j == null) {
				throw ''
			}
		}
		catch (e) {
			console.error('Error parsing ' + file + ': ' + e)
			error_callback()
			return
		}

        callback(j)
	}, error_callback, method, parameters)
}

function loadStaticTextFile(path, callback, error_callback) {
	let xhttp = new XMLHttpRequest();

	xhttp.onreadystatechange = function() {
	    if (this.readyState == 4) {
	        if(this.status == 200) {
	            callback(this.responseText)
	        }
	        else {
	            if(error_callback != null) {
	            	error_callback(this.status)
	            }
	            else {
	            	console.error('Error loading ' + path + ': ' + this.status);
	            }
	        }
	    }
	};
	xhttp.open('GET', path, true);
	xhttp.send();
}


function filterHTML(unsafe, convertNewlines=false, formatting=false) {
	if(unsafe == null) {
		return ''
	}
	let safe = unsafe
	     .replace(/&/g, "&amp;")
	     .replace(/</g, "&lt;")
	     .replace(/>/g, "&gt;")
	     .replace(/"/g, "&quot;")
	     .replace(/'/g, "&#039;")

	if(convertNewlines) {
		safe = safe.replace(/\n/g, "<br>")
	}
	if(formatting) {
		safe = safe
                .replace(/__\*\*\*([^*]+?)\*\*\*__/g, "<u><b><i>$1</i></b></u>")
                .replace(/\*\*\*([^*]+?)\*\*\*/g, "<b><i>$1</i></b>")
                .replace(/__\*\*([^*]+?)\*\*__/g, "<u><b>$1</b></u>")
                .replace(/__\*([^*]+?)\*__/g, "<u><i>$1</i></u>")
                .replace(/\*\*([^*]+?)\*\*/g, "<b>$1</b>")
				.replace(/\*([^*]+?)\*/g, "<b>$1</b>")
				.replace(/__([^*]+?)__/g, "<u>$1</u>")
				.replace(/_([^*]+?)_/g, "<i>$1</i>")
                .replace(/~~([^*]+?)~~/g, "<del>$1</del>")
	}
	return safe
}


function isChildOf(parent, child) {
	let p = child.parentNode
	while(p != null) {
		if(p == parent) {
			return true
		}

		p = p.parentNode
	}
	return false
}

function isOrIsChildOf(parent, child) {
	return (parent == child) || isChildOf(parent, child)
}

function secondsToTimeString(t) {
	t = Math.abs(t)
	if(t <= 0) {
		return 'Less than one second'
	}
	if(t < 60) {
		return 'Less than 1 minute'
	}
	if(t < 60*60) {
		if(t < 2*60) {
			return '1 minute'
		}
		return Math.floor(t / 60) + ' minutes'
	}
	if(t < 24*60*60) {
		if(t < 2*60*60) {
			return '1 hour'
		}
		return Math.floor(t / 60 / 60) + ' hours'
	}
	if(t < 7*24*60*60) {
		if(t < 2*24*60*60) {
			return '1 day'
		}
		return Math.floor(t / 60 / 60 / 24) + ' days'
	}
	if(t >= 365*24*60*60) {
		if(t < 2*365*24*60*60) {
			return '1 year'
		}
		return Math.floor(t / 60 / 60 / 24 / 365) + ' years'
	}
	if(t < 2*7*24*60*60) {
		return '1 week'
	}
	return Math.floor(t / 60 / 60 / 24 / 7) + ' weeks'
}

// Takes DOM input element with type='file', url without the '.json'
// and args e.g. '&event_id=2&foo=3'
// TODO: Proper error handling
function uploadFile(file, url, args, success_callback, error_callback) {
	if(args == null) {
		args = ''
	}

	var file_name = ''

	if(file.value.indexOf('\\') > -1) {
		var file_split = file.value.split('\\')
		file_name = file_split[file_split.length-1]
	}
	else {
		var file_split = file.value.split('/')
		file_name = file_split[file_split.length-1]
	}

	var reader = new FileReader();
	reader.onload = function(e) {
	    var blob = new Blob([new Uint8Array(e.target.result)], {type: file.type });
	    

		loadJSON(url, x => {
			var http_s3 = new XMLHttpRequest();
			http_s3.open('PUT',x.presignedURL, true);
			http_s3.setRequestHeader('x-amz-acl', 'public-read')
			http_s3.setRequestHeader('cache-control', 'public, max-age=31536000, immutable')
			http_s3.setRequestHeader('content-disposition', 'attachment; filename="' + file_name + '"')
			

			// TODO http_s3.setRequestHeader('content-length', blob.length)
			http_s3.onreadystatechange = function() {
			    if(http_s3.readyState == 4) {
			    	if(http_s3.status == 200) {
				        success_callback({id: x.id, fileName: file_name})
				    }
				    else {
			    		error_callback('Error uploading to S3. HTTP Error Code '+http_s3.status)
				    }
			    }
			}
			http_s3.send(blob)

		}, e => error_callback('HTTP Error: '+e), 'POST', 'file_name=' + encodeURIComponent(file_name) + args)
	};
	reader.readAsArrayBuffer(file.files[0]);
}

// https://stackoverflow.com/a/30832210/11498001
// mime_type: text/plain, text/calendar
function downloadFile(filename, contents, mime_type) {
	var file = new Blob([contents], {type: mime_type});
    if (window.navigator.msSaveOrOpenBlob) // IE10+
        window.navigator.msSaveOrOpenBlob(file, filename);
    else { // Others
        let a = document.createElement("a"),
        	url = URL.createObjectURL(file);
        a.href = url;
        a.download = filename;
        document.body.appendChild(a);
        a.click();
        setTimeout(function() {
            document.body.removeChild(a);
            window.URL.revokeObjectURL(url);  
        }, 0); 
    }
}
