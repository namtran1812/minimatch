export default function VideoBackground() {
  return (
    <div className="video-background" aria-hidden="true">
      <video
        className="video-background-fill"
        autoPlay
        muted
        loop
        playsInline
        preload="auto"
      >
        <source
          src="/background.mp4"
          type="video/mp4"
        />
      </video>

      <video
        className="video-background-main"
        autoPlay
        muted
        loop
        playsInline
        preload="auto"
      >
        <source
          src="/background.mp4"
          type="video/mp4"
        />
      </video>

      <div className="video-background-vignette" />
    </div>
  );
}
