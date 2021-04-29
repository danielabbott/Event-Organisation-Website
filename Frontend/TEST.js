async function ___generateUsers() {
	let first_names = ['Aaron', 'Buck', 'Camilla', 'Daphne', 'Ellen', 'Jacob']
	let last_names = ['Addams', 'Baker', 'Coleman', 'Devon', 'Erridge', 'Williamson']

	for(let i = 0; i < 1000; i++) {
		let formData = 'email=' + i + '@phonyemail.notreal'
			+ '&pwd=12345678'+i
			+ '&name=' + encodeURIComponent(first_names[i % 6] + ' ' + last_names[i % 6])

		loadJSON('signup', _ => {}, e => { console.error(e)}, 'POST', formData)

		await new Promise(r => setTimeout(r, 2));
	}
}


function ___generateExampleData() {
	___generateUsers()
}