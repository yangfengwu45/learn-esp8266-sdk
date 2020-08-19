const formatTime = date => {
  const year = date.getFullYear()
  const month = date.getMonth() + 1
  const day = date.getDate()
  const hour = date.getHours()
  const minute = date.getMinutes()
  const second = date.getSeconds()

  return [year, month, day].map(formatNumber).join('/') + ' ' + [hour, minute, second].map(formatNumber).join(':')
}

const formatNumber = n => {
  n = n.toString()
  return n[1] ? n : '0' + n
}

// util.newAb2Str代码
var newAb2Str = function newAb2Str(arrayBuffer) {
  let unit8Arr = new Uint8Array(arrayBuffer);
  let encodedString1 = String.fromCharCode.apply(null, unit8Arr);
  let decodedString = decodeURIComponent(escape((encodedString1)));//没有这一步中文会乱码
  return decodedString;
}

module.exports = {
  formatTime: formatTime,
  newAb2Str: newAb2Str
}
