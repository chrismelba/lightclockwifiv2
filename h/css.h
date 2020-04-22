const char css_file[] PROGMEM = R"=====(
/*  ==========================================================================
    BEGIN FONTS
    ========================================================================== */
  @import url(http://fonts.googleapis.com/css?family=Abel);
  @import url(https://maxcdn.bootstrapcdn.com/font-awesome/4.3.0/css/font-awesome.min.css);

/*  ==========================================================================
    BEGIN RESET
    ========================================================================== */
      *{-webkit-box-sizing:border-box; -moz-box-sizing:border-box; box-sizing:border-box;}
    html{ /* Permalink - use to edit and share this gradient: http://colorzilla.com/gradient-editor/#e0e0e0+0,ffffff+50,e0e0e0+100 */
background: #e0e0e0; /* Old browsers */
background: -moz-linear-gradient(left,  #e0e0e0 0%, #ffffff 50%, #e0e0e0 100%); /* FF3.6+ */
background: -webkit-gradient(linear, left top, right top, color-stop(0%,#e0e0e0), color-stop(50%,#ffffff), color-stop(100%,#e0e0e0)); /* Chrome,Safari4+ */
background: -webkit-linear-gradient(left,  #e0e0e0 0%,#ffffff 50%,#e0e0e0 100%); /* Chrome10+,Safari5.1+ */
background: -o-linear-gradient(left,  #e0e0e0 0%,#ffffff 50%,#e0e0e0 100%); /* Opera 11.10+ */
background: -ms-linear-gradient(left,  #e0e0e0 0%,#ffffff 50%,#e0e0e0 100%); /* IE10+ */
background: linear-gradient(to right,  #e0e0e0 0%,#ffffff 50%,#e0e0e0 100%); /* W3C */
filter: progid:DXImageTransform.Microsoft.gradient( startColorstr='#e0e0e0', endColorstr='#e0e0e0',GradientType=1 ); /* IE6-9 */
}
    body{text-align:center; margin: 0; padding: 40px; background: #fff; box-shadow: 0 1px 10px rgba(0, 0, 0, 0.1); border-radius: 3px; font-family: 'Abel', sans-serif;}
    ul{ margin: 0; padding: 0; list-style: none;}
    a{ text-decoration: none;}
    label{ font-weight: bold;}
    input,select{ background: #fff; text-transform: capitalize; margin: 0; border: 1px solid rgba(0,0,0,0.15); border-radius: 3px; font-family: 'Abel',sans-serif; font-size: 15px; letter-spacing: 1px; display: inline-block; padding: 15px; -webkit-transition: all 0.6s ease 0s; -moz-transition: all 0.6s ease 0s; -ms-transition: all 0.6s ease 0s; -o-transition: all 0.6s ease 0s; transition: all 0.6s ease 0s;}
    input:focus,select:focus{ border-color: rgba(0,0,0,0.5); outline: none;}
    select option{ padding: 0 10px;}

/*  ==========================================================================
    BEGIN FORM
    ========================================================================== */
    .settings-page{ width: 550px; margin: 60px auto;}
    .form-verticle ul li{ width: 100%; display: inline-block; margin-bottom: 30px; margin-top: 0; position: relative;}
    .form-verticle ul li label{ width: 35%; display: inline-block; color: #222; text-transform: uppercase; letter-spacing: 1.5px; line-height: 48px; float: left;}
    .form-verticle ul li .form-field{ width: 65%; display: inline-block; float: left;}
    .form-verticle ul li .form-field select,.form-verticle ul li .form-field input{ width: 100%;}
    .form-verticle ul li.checkbox input[type="checkbox"]{ display: none;}
    .form-verticle ul li.checkbox label {display: inline-block; cursor: pointer; font-weight: bold; padding-left: 0; line-height: 17px;}
    .form-verticle ul li.checkbox label:before { color: #fff; content: "\f00c"; font-family: FontAwesome; font-size: 13px; text-align: center; line-height: 20px; display: inline-block; width: 23px; height: 22px; margin-right: 10px; position: absolute; left: 35%; top: -2px; border: 1px solid rgba(0,0,0,0.15); -webkit-transition: all 0.35s ease 0s; -moz-transition: all 0.35s ease 0s; -ms-transition: all 0.35s ease 0s; -o-transition: all 0.35s ease 0s; transition: all 0.35s ease 0s;}
    .form-verticle ul li.checkbox input[type="checkbox"]:checked + label:before { color: #333; border-color: rgba(0,0,0,0.5);}
    .form-verticle ul li input.btn{ width: auto;}
    .btn{ background: #F0F0F0; color: #111; text-align: center; width: auto; cursor: pointer; padding: 15px 20px; text-transform: uppercase; border-radius: 3px; font-family: 'Abel',sans-serif; font-size: 18px; letter-spacing: 1px; font-weight: bold; display: inline-block; border: 0; -webkit-transition: all 0.5s ease 0s; -moz-transition: all 0.5s ease 0s; -ms-transition: all 0.5s ease 0s; -o-transition: all 0.5s ease 0s; transition: all 0.5s ease 0s}
    .btn-sm{ padding: 5px 10px !important; font-size: 13px;}
    .btn:hover{ background: #111; color: #fff;}
    .btn-default{ background: #111; color: #fff;}
    .btn-default:hover{ background: #F0F0F0; color: #111;}
    .btn-green{ background: #35CD56; color: #fff;}
    .btn-green:hover{ background: #111; color: #fff;}
    .btn-box{ display: flex; justify-content: center;padding: 10px;}
    .btn-box + .btn-box{ }
    .btn-box .btn{ font-size: 14px; margin: 0 5px; display: flex; align-items: center;}
    .btn-box .btn-sm{ font-size: 13px;}
    .section-head{text-transform: uppercase; letter-spacing: 1.5px; line-height: 48px;}

    .color-section{ width: 100%; display: inline-block; vertical-align: top; margin-bottom: 30px;}
    .color-box{ width: 50%; display: inline-block; vertical-align: top; float: left;}
    .color-box > label {display: inline-block; margin-bottom: 10px; width: 100%;}
    .slide-section{ width: 100%; display: inline-block; vertical-align: top; margin-bottom: 30px;}
    .point-slide{ width: 50%; display: inline-block; vertical-align: top; float: left;}
    .point-slide > label {display: inline-block; margin-bottom: 10px; width: 100%;}
    .btn-footer{
      z-index: 10000000;
      margin: 0px auto;
      border-top: 1px solid rgba(0, 0, 0, 0.1);
        
        padding: 30px 40px 0;
        
            position: fixed;
        left: 0;
        right: 0;
        bottom: 0;
        height: 100px;
        background: #ffffff;
    }

.bottomspacer {
  height: 100px;
}
.sp-container {
    position:absolute;
    top:0;
    left:0;
    display:inline-block;
    *display: inline;
    *zoom: 1;
    /* https://github.com/bgrins/spectrum/issues/40 */
    z-index: 9999994;
    overflow: hidden;
}
.sp-picker-container {
    width: 172px;
    border-left: solid 1px #fff;
}
    .sp-input-container{
      margin-bottom: 8px;
    }
.sp-input {
   font-size: 12px !important;
   border: 1px inset;
   padding: 4px 5px;
   margin: 0;
   width: 100%;
   background:transparent;
   border-radius: 3px;
   color: #222;
}
    .sp-input:focus{
      border-color: rgba(0, 0, 0, 0.5);
    }
    .sp-dragger{
      height: 10px;
      width: 10px;
      background: #fff;
      border: 1px solid;
      border-radius: 5px;
    }


/*  ==========================================================================
    RESPONSIVE
  ========================================================================== */   
    @media screen and (max-width: 550px){
      .settings-page{ width: auto; margin: 20px;}
      .color-box{ width: 100%; margin-bottom: 20px;}
      .point-slide{ width: 100%; margin-bottom: 20px;}
    }
    @media screen and (max-width: 500px){
      .form-verticle ul li label{ width: 100%;}
      .form-verticle ul li .form-field{ width: 100%;}
      .hide-mobile{ display: none!important;}
      .btn-box{ display: inline-block; width: 100%;}
      .btn-box .btn{ margin: 0 0 3px;}
      .form-verticle ul li.checkbox label:before{left: auto; right: 0;}
      .btn-box .btn-sm{ width: 100%; display: inline-block;}
    }
    @media screen and (max-width: 480px){
      .settings-page{padding: 10px 20px;}
      .btn-footer{ margin: 20px -20px 0; padding: 20px 20px 0;}
      .btn-footer .btn{padding: 8px; height: 30px ; width: 100%; display: inline-block; margin-bottom: 5px;font-size: 12px;}
    }
    #rcorners2 {
        border-radius: 25px;
        border: 2px solid #AAAAAA;
        padding: 20px; 

    }

    .tooltip{
        display: inline;
        position: relative;
    }
    
    .tooltip:hover:after{
        background: #333;
        background: rgba(0,0,0,.8);
        border-radius: 5px;
        bottom: 26px;
        color: #fff;
        content: attr(title);
        left: 20%;
        padding: 5px 15px;
        position: absolute;
        z-index: 98;
        width: 220px;
    }
    
    .tooltip:hover:before{
        border: solid;
        border-color: #333 transparent;
        border-width: 6px 6px 0 6px;
        bottom: 20px;
        content: "";
        left: 50%;
        position: absolute;
        z-index: 99;
    }   
)=====";
