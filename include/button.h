#ifndef button_BaoTran97_h                                                                    // nếu thư viện chưa được đọc
#define button_BaoTran97_h                                                                    // đánh dấu đã đọc


#define BUTTON_ANALOG  1                                                                      // định nghĩa             
#define BUTTON_DIGITAL 0                                                                      // định nghĩa             

class Button {                                                                                // lớp hàm quy ước cách sử lý nút nhấn
  public :                                                                                    // biến hàm toàn cục
    int           type_button = BUTTON_DIGITAL;                                               // biến kiểu nút nhấn
    unsigned int  max_analog  = 4095;                                                         // biến giá trị tối đa
    int           pin;                                                                        // chân đọc tín hiệu
    int           val_reference;                                                              // giá trị tham chiếu
    double        delta;                                                                      // giá trị độ lệch cho phép
    boolean       old_stt;                                                                    // giá trị đọc cũ
    boolean       read_return;                                                                    // giá trị đọc cũ được trả về
    //boolean       old_stt_falling_delay;                                                      // thời gian nút được nhấn
    //boolean       old_stt_rising_delay;                                                       // thời gian nút được nhả
    unsigned long time_Falling;                                                               // thời gian nút được nhấn
    unsigned long time_Rising;                                                                // thời gian nút được nhả
    unsigned long time_refresh;                                                               // thời gian nút nhấn được nhận gần nhất
    unsigned long time_true;                                                                  // thời gian nút được xác nhận là nhấn
    unsigned long time_false;                                                                  // thời gian nút được xác nhận là nhả

    Button(long _pin, int _type = BUTTON_DIGITAL, unsigned long _high_resistor = 0, unsigned long _low_resistor = 0, double _delta = 20) {
      pin             = _pin;                                                                 // lưu chân đọc giá trị
      type_button     = _type;                                                                // lưu kiểu nút nhấn

      if (type_button == BUTTON_DIGITAL) {                                                    // nếu kiểu nút nhấn là digital
        pinMode(pin, INPUT_PULLUP);                                                           // đặt chân là đầu vào với điện trở kéo lên
        val_reference = 0;                                                                    // giá trị so sánh khi nhận tín hiệu ở mức 0
      } else {                                                                                // nếu là nút nhấn analog
        val_reference = (double(max_analog) * double(_low_resistor)) / double(_high_resistor + _low_resistor); // tính giá trị so sánh dựa vào cầu phân áp
      }                                                                                       //
      delta           = max_analog * _delta / 100.0;                                          // tính sai số cho phép
    }                                                                                         //



    boolean read(unsigned long _time_delay = 100) {                                           // hàm đọc trạng thái nút nhấn
      int val;                                                                                // biến lưu giá trị đọc
      if (type_button == BUTTON_DIGITAL)  val = digitalRead(pin) * max_analog;                // nếu là nút nhấn digital thì trả về giá trị là 0 hoặc max analog
      else                                val = analogRead (pin);                             // nếu là nút nhấn analog thì trả về giá trị analog đọc được

      if (abs(val - val_reference) <= delta) {                                                // nếu sai số trong khoảng cho phép
        if (time_true <= time_false) time_true = millis();                                    // nếu giá trị mới đúng lần đầu thì đánh dấu thời gian đúng
      } else {                                                                                // nếu sai số vượt mức cho phép
        if (time_false <= time_true) time_false = millis();                                   // nếu giá trị mới sai lần đầu thì đánh dấu thời gian sai
      }                                                                                       //

      if (time_true  > millis()) time_true = 0;                                               // reset vòng lặp thời gian dài
      if (time_false > millis()) time_true = 0;                                               // reset vòng lặp thời gian dài

      if ((time_true > time_false) && (millis() - time_true  > _time_delay)) read_return = 0; // nếu đúng trong khoảng thời gian đủ lâu trả về giá trị đúng
      if ((time_false > time_true) && (millis() - time_false > _time_delay)) read_return = 1; // nếu sai trong khoảng thời gian đủ lâu trả về giá trị sai

      return read_return;                                                                     // trả về
    }                                                                                         //

    boolean IsFalling(unsigned long _time_delay = 50) {                                       // hàm nhận xung cạnh xuống
      boolean new_stt = read();                                                               // đọc giá trị
      boolean falling = (old_stt) && (!new_stt);                                              // kiểm tra xem có phải cạnh xuống hay không
      boolean Rising  = (!old_stt) && (new_stt);                                              // kiểm tra xem có phải cạnh lên hay không
      old_stt         = new_stt;                                                              // cập nhật biến trạng thái

      if (Rising) time_Rising = millis();                                                     // nếu nút nhấn được nhả thì đánh dấu là đã nhả nút nhấn

      if (falling) {                                                                          // nếu là xung cạnh xuóng
        time_Falling  = millis();                                                             // đánh dấu thời gian xung cạnh xuống
        time_refresh  = millis();                                                             // đánh dấu thời gian nhận nút nhấn
        if (millis() - time_Rising < _time_delay) time_Rising = 0;                            // nếu tín hiệu nhả nút nhấn không đủ thời gian lặp thì hủy
      }                                                                                       //

      if ((millis() - time_Falling >= _time_delay) && (time_Rising) && (!new_stt)) {          // nếu đạt thời gian delay cần thiết và nút nhấn đang được nhấn
        time_Rising = 0;                                                                      // đánh dấu là đã tạo xung clock
        return 1;                                                                             // trả về xung clock sự kiện nhấn nút
      }                                                                                       //

      return 0;                                                                               // trả về không có sự kiện nhấn nút
    }                                                                                         //

    boolean IsFallingContinuous(unsigned long _time_refresh = 100, unsigned long _time_delay = 500) {// hàm nhấn giữ lặp lại trong khoảng thời gian
      if (IsFalling()) return 1;                                                              // nếu có sự kiện nhấn nút trả về sự xung clock sự kiện
      if (read())      return 0;                                                              // nếu nút không được nhấn thì thoát hàm

      if ((millis() - time_Falling >= _time_delay) && (millis() - time_refresh >= _time_refresh)) {// nếu đủ thời gian delay cần thiết và đủ thời gian lặp lại
        time_refresh = millis();                                                              // đánh dấu thời gian lặp lại
        return 1;                                                                             // trả về xung clock sự kiện
      }                                                                                       //

      return 0;                                                                               // còn lại không có xung clock sự kiện nào cần được thực thi
    }                                                                                         //

    boolean IsLow() {                                                                         // hàm kiểm tra có phải mức thấp không
      return !read();                                                                         // trả về true khi nút được nhấn
    }                                                                                         //

    boolean IsHigh() {                                                                        // hàm kiểm tra có phải mức cao không
      return read();                                                                          // trả về true khi nút không được nhấn
    }                                                                                         //
};                                                                                            //


#endif
