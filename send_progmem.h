template<size_t N>
size_t sendProgmem(WiFiClient client, const prog_char (&str)[N])
{
  return sendProgmem(client, str, N);
}

size_t sendProgmem(WiFiClient client, const prog_char progmem[], size_t size) {
    size_t maxSize=2920; // this seems to be the max that write() will ever actually send
    size_t sent=0;

    while(sent < size) {
      size_t sendSize = (maxSize < (size - sent)) ? maxSize : (size - sent);
      Serial.println(sendSize);
      Serial.print("heap: ");
      Serial.println(ESP.getFreeHeap());
      sent += client.write(progmem+sent, sendSize); // write() may send less than you asked it to
    }
    return sent;
}

