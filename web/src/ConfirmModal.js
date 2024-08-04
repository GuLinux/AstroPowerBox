import Modal from 'react-bootstrap/Modal';
import Button from 'react-bootstrap/Button';
import { useState } from 'react';

export const ConfirmModal = ({
        RenderButton,
        onCancel=() => {},
        onConfirm,
        title="Confirm?",
        text="Do you want to confirm?",
        cancelButton="Cancel",
        confirmButton="Confirm",
    }) => {
    const [show, setShow] = useState(false);
    const closeModal = () => setShow(false);
    const cancelModal = () => {
        onCancel();
        closeModal();
    }
    const confirmModal = () => {
        onConfirm();
        closeModal();
    }
    return <>
        <RenderButton onClick={() => setShow(true)} />
        <Modal show={show} onHide={onCancel}>
            <Modal.Header closeButton>
                <Modal.Title>{title}</Modal.Title>
            </Modal.Header>
            <Modal.Body>
                {text}
            </Modal.Body>
            <Modal.Footer>
                <Button variant="secondary" onClick={cancelModal}>{cancelButton}</Button>
                <Button variant="primary" onClick={confirmModal}>{confirmButton}</Button>
            </Modal.Footer>
        </Modal>
    </>
}

