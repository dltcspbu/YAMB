(async () => {
  document.addEventListener("DOMContentLoaded", async () => {
    while (true) {

      // $('#main').empty()
      $('div[secret]').empty()
      let response = await fetch('/check')
      let logData = await response.json()

      logData = logData.sort((a, b) => (a.roomId > b.roomId) ? 1 : ((b.roomId > a.roomId) ? -1 : 0));

      let tmpItem = {}
      tmpItem['roomId'] = ''
      logData.forEach((item) => {

        if (item.roomId !== tmpItem.roomId) {
          let button = $(`<input style="display: block;" class="${item.roomId}" type="button" value="${item.roomId}" />`)
          let output = $(`<div class="${item.roomId}" secret hidden="true"></div>`)
          if (!$(`input[class="${item.roomId}"]`).length) {
            $('#main').append(button)

            $(`input[class="${item.roomId}"]`).click(function () {
              $(`div[class="${item.roomId}"]`).toggle()
            })
          }

          if (!$(`div[class="${item.roomId}"]`).length)
            $('#main').append(output)

          let template = $('#log-template').html()
          let clonedTemplate = $(template).clone()

          $(clonedTemplate).find('.from').html(item.from)
          $(clonedTemplate).find('.class').html(item.class)
          $(clonedTemplate).find('.timestamp').html(item.timestamp)

          $(`div[class="${item.roomId}"]`).append(clonedTemplate)
          tmpItem = item
        } else {
          let template = $('#log-template').html()
          let clonedTemplate = $(template).clone()

          $(clonedTemplate).find('.from').html(item.from)
          $(clonedTemplate).find('.class').html(item.class)
          $(clonedTemplate).find('.timestamp').html(item.timestamp)

          $(`div[class="${item.roomId}"]`).append(clonedTemplate)
        }
      })

      await new Promise(resolve => setTimeout(resolve, 10000))
    }

  });
})();
